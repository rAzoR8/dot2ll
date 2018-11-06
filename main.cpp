#include "DotParser.h"
#include "DotWriter.h"
#include "Dot2CFG.h"
#include "CFG2Dot.h"
#include "OpenTree.h"
#include "InstructionSetLLVMAMD.h"
#include "CheckReconvergence.h"
#include <filesystem>

static const std::wstring OrderNames[] =
{
    L"DepthFirst",
    L"BreadthFirst",
    L"DepthFirstDom",
    L"All",
    L"Custom"
};

void dot2ll(const std::string& _sDotFile, const NodeOrdering::Type _kOrder, const bool _bReconv, const std::filesystem::path& _sOutPath, const bool _bPutVirtualFront = false, const std::string& _sCustomOrder = {})
{
    DotGraph dotin = DotParser::ParseFromFile(_sDotFile);

    if (dotin.GetNodes().empty() == false)
    {
        Function func = Dot2CFG::Convert(dotin);

        if (func.EnforceUniqueExitPoint() == false)
            return;

        const bool bInputReconverging = CheckReconvergence::IsReconverging(func);

        HLOG("Reconverging %s '%s' [Order: %s Reconv: %s]", WCSTR(_sDotFile), WCSTR(dotin.GetName()), WCSTR(OrderNames[_kOrder]), bInputReconverging ? L"true" : L"false");

        std::string sOutName = dotin.GetName();

        if (_bReconv)
        {
            sOutName += "_reconv";

            NodeOrder InputOrdering;

            switch (_kOrder)
            {
            case NodeOrdering::DepthFirst:
                InputOrdering = NodeOrdering::ComputeDepthFirst(func.GetEntryBlock());
                sOutName += "_df";
                break;
            case NodeOrdering::BreadthFirst:
                InputOrdering = NodeOrdering::ComputeBreadthFirst(func.GetEntryBlock());
                sOutName += "_bf";
                break;
            case NodeOrdering::Custom:
                InputOrdering = NodeOrdering::ComputeCustomOrder(func.GetCFG(), _sCustomOrder);
                sOutName += "_custom";
                break;
            case NodeOrdering::DepthFirstDom:
            default:
                InputOrdering = NodeOrdering::ComputePaper(func.GetEntryBlock(), func.GetExitBlock());
                sOutName += "_dfd";
                break;
            }

            // execute prepare pass even if input ordering is already reconverging (for debugging purpose)
            const bool bPrepareIfReconv = true;

            // reconverge using InputOrdering
            OpenTree OT(true ,_sOutPath.string() + "/");
            const bool bChanged = OT.Process(InputOrdering, bPrepareIfReconv, _bPutVirtualFront);

            func.Finalize();

            const bool bOutputReconverging = CheckReconvergence::IsReconverging(func, true);
            hlx::Logger::Instance()->Log(bOutputReconverging ? hlx::kMessageType_Info : hlx::kMessageType_Error, WFUNC, WFILE, __LINE__, L"Function %s reconverging!\n", bOutputReconverging ? L"is" : L"is NOT");
            
            std::ofstream dotout(_sOutPath / (sOutName + ".dot"));

            if (dotout.is_open())
            {
                DotWriter::WriteToStream(CFG2Dot::Convert(func.GetCFG(), func.GetName()), dotout);

                dotout.close();
            }

            assert(bOutputReconverging);

            // if the input was already reconverging, the algo should not change the CFG.
            // (but in the case where virtual nodes are added to the ordering, this does not hold in the current definition and implemention)
            assert(bInputReconverging ? !bChanged || bPrepareIfReconv : bChanged);
        }

        std::ofstream ll(_sOutPath / (sOutName + ".ll"));

        if (ll.is_open())
        {
            func.Finalize();

            InstructionSetLLVMAMD isa;
            isa.SerializeListing(func, ll);

            ll.close();
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    NodeOrdering::Type kOrder = NodeOrdering::DepthFirstDom;
    hlx::Logger::Instance()->WriteToStream(&std::wcout);

    std::filesystem::path InputPath;
    std::filesystem::path OutPath;
    std::string sCustomOrder;

    bool bReconv = false;
    bool bVirtualFront = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string token(argv[i]);

        if (token == "-r")
        {
            bReconv = true;
        }
        else if (token == "-depthfirst")
        {
            kOrder = NodeOrdering::DepthFirst;
        }
        else if (token == "-depthfirstdom")
        {
            kOrder = NodeOrdering::DepthFirstDom;
        }
        else if (token == "-breadthfirst")
        {
            kOrder = NodeOrdering::BreadthFirst;
        }
        else if (token == "-all")
        {
            kOrder = NodeOrdering::All;
        }
        else if (token == "-virtualfront")
        {
            bVirtualFront = true;
        }
        else if (token == "-custom" && (i + 1) < argc)
        {
            kOrder = NodeOrdering::Custom;
            sCustomOrder = argv[++i];
        }
        else if (token == "-out" && (i+1) < argc)
        {
            OutPath = argv[++i];
        }
        else
        {
            InputPath = token;
            if (OutPath.empty())
            {
                OutPath = std::filesystem::is_directory(InputPath) ? InputPath : InputPath.parent_path();
            }
        }
    }

    const auto Reconv = [&](NodeOrdering::Type _kOrder)
    {
        if (std::filesystem::is_directory(InputPath))
        {
            for (const auto& Entry : std::filesystem::directory_iterator(InputPath))
            {
                if (Entry.is_directory() == false && Entry.path().extension() == ".dot")
                {
                    dot2ll(Entry.path().string(), _kOrder, bReconv, OutPath, bVirtualFront, sCustomOrder);
                }
            }
        }
        else
        {
            dot2ll(InputPath.string(), _kOrder, bReconv, OutPath, bVirtualFront, sCustomOrder);
        }
    };

    if (kOrder == NodeOrdering::All)
    {
        for (uint32_t i = 0u; i < NodeOrdering::All; ++i)
        {
            Reconv(NodeOrdering::Type(i));
        }
    }
    else
    {
        Reconv(kOrder);
    }

    return 0;
}
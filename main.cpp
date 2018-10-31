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
};

void dot2ll(const std::string& _sDotFile, const NodeOrdering::Type _kOrder, const bool _bReconv = false, const std::filesystem::path& _sOutPath = {})
{
    DotGraph dotin = DotParser::ParseFromFile(_sDotFile);

    if (dotin.GetNodes().empty() == false)
    {
        Function func = Dot2CFG::Convert(dotin);

        const bool bInputReconverging = CheckReconvergence::IsReconverging(func, !_bReconv, true);

        HLOG("Reconverging %s '%s' [Order: %s Reconv: %s]", WCSTR(_sDotFile), WCSTR(dotin.GetName()), WCSTR(OrderNames[_kOrder]), bInputReconverging ? L"true" : L"false");

        std::string sOutName = dotin.GetName();

        if (_bReconv)
        {
            sOutName += "_reconv";

            func.EnforceUniqueExitPoint();

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
            case NodeOrdering::DepthFirstDom:
            default:
                InputOrdering = NodeOrdering::ComputePaper(func.GetEntryBlock(), func.GetExitBlock());
                sOutName += "_dfd";
                break;
            }

            // reconverge using InputOrdering
            OpenTree OT;
            OT.Process(InputOrdering);

            func.Finalize();

            const bool bOutputReconverging = CheckReconvergence::IsReconverging(func, true);
            hlx::Logger::Instance()->Log(bOutputReconverging ? hlx::kMessageType_Info : hlx::kMessageType_Error, WFUNC, WFILE, __LINE__, L"Function %s reconverging!\n", bOutputReconverging ? L"is" : L"is NOT");

            HASSERTD(bOutputReconverging, "Function %s is NOT reconverging!", WCSTR(func.GetName()));

            std::ofstream dotout(_sOutPath / (sOutName + ".dot"));

            if (dotout.is_open())
            {
                DotWriter::WriteToStream(CFG2Dot::Convert(func.GetCFG(), func.GetName()), dotout);

                dotout.close();
            }
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
    hlx::Logger::Instance()->WriteToStream(&std::wcout);

    std::filesystem::path InputPath;
    std::filesystem::path OutPath;

    if (argc < 2)
        return 1;

    NodeOrdering::Type kOrder = NodeOrdering::DepthFirstDom;

    bool bReconv = false;

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
        else if (token == "-out" && (i+1) < argc)
        {
            OutPath = argv[i+1];
            i++;
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
                    dot2ll(Entry.path().string(), _kOrder, bReconv, OutPath);
                }
            }
        }
        else
        {
            dot2ll(InputPath.string(), _kOrder, bReconv, OutPath);
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
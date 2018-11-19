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
    L"DepthFirstDom",
    L"BreadthFirst",
    L"BreadthFirstDom",
    L"RPOT",
    L"Custom"
};

void dot2ll(const std::string& _sDotFile, const uint32_t _uOderIndex, const bool _bReconv, const std::filesystem::path& _sOutPath, const bool _bPutVirtualFront, const bool _bCloseSuccAfterAddNode, const std::string& _sCustomOrder)
{
    const NodeOrdering::Type _kOrder{ 1u << _uOderIndex };

    DotGraph dotin = DotParser::ParseFromFile(_sDotFile);

    const size_t uUserNodes = dotin.GetNodes().size();

    if (uUserNodes == 0u)
    {
        HERROR("Failed to parse %s", WCSTR(_sDotFile));
        return;
    }

    Function func = Dot2CFG::Convert(dotin);

    if (func.EnforceUniqueEntryPoint() == false || func.EnforceUniqueExitPoint() == false)
        return;

    const bool bInputReconverging = CheckReconvergence::IsReconverging(func);

    HLOG("Processing %s '%s' [Order: %s Reconv: %s]", WCSTR(_sDotFile), WCSTR(dotin.GetName()),
        _kOrder == NodeOrdering::Custom ? WCSTR(_sCustomOrder) : WCSTR(OrderNames[_uOderIndex]), bInputReconverging ? L"true" : L"false");

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
            InputOrdering = NodeOrdering::ComputeBreadthFirst(func.GetEntryBlock(), false);
            sOutName += "_bf";
            break;
        case NodeOrdering::BreadthFirstDom:
            InputOrdering = NodeOrdering::ComputeBreadthFirst(func.GetEntryBlock(), true);
            sOutName += "_bfd";
            break;
        case NodeOrdering::RPOT:
            InputOrdering = NodeOrdering::ComputeReversePostorderTraversal(func.GetExitBlock());
            sOutName += "_rpot";
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

        if (InputOrdering.size() != uUserNodes)
        {
            HFATALD("Orderin is not a valid traversal of the input CFG!");
            return;
        }

        // execute prepare pass even if input ordering is already reconverging (for debugging purpose)
        const bool bPrepareIfReconv = true;

        // reconverge using InputOrdering
        OpenTree OT(true, _sOutPath.string() + "/");
        const bool bChanged = OT.Process(InputOrdering, bPrepareIfReconv, _bPutVirtualFront, _bCloseSuccAfterAddNode);

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

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    uint32_t kOrder = NodeOrdering::None;
    hlx::Logger::Instance()->WriteToStream(&std::wcout);

    std::filesystem::path InputPath;
    std::filesystem::path OutputPath;
    std::string sCustomOrder;

    bool bReconv = false;
    bool bVirtualFront = false;
    bool bCloseSuccAfterAddNode = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string token(argv[i]);

        if (token == "-r")
        {
            bReconv = true;
        }
        else if (token == "-depthfirst")
        {
            kOrder |= NodeOrdering::DepthFirst;
        }
        else if (token == "-depthfirstdom")
        {
            kOrder |= NodeOrdering::DepthFirstDom;
        }
        else if (token == "-breadthfirst")
        {
            kOrder |= NodeOrdering::BreadthFirst;
        }
        else if (token == "-breadthfirstdom")
        {
            kOrder |= NodeOrdering::BreadthFirstDom;
        }
        else if (token == "-breadthfirstdom")
        {
            kOrder |= NodeOrdering::RPOT;
        }
        else if (token == "-all")
        {
            kOrder = NodeOrdering::All;
        }
        else if (token == "-custom" && (i + 1) < argc)
        {
            kOrder |= NodeOrdering::Custom;
            sCustomOrder = argv[++i];
        }
        else if (token == "-virtualfront")
        {
            bVirtualFront = true;
        }
        else if (token == "-closeafteradd")
        {
            bCloseSuccAfterAddNode = true;
        }
        else if (token == "-out" && (i + 1) < argc)
        {
            OutputPath = argv[++i];
        }
        else if (token == "-in" && (i + 1) < argc)
        {
            InputPath = argv[++i];
        }
        else
        {
            InputPath = token;
        }
    }

    if (OutputPath.empty())
    {
        OutputPath = std::filesystem::is_directory(InputPath) ? InputPath : InputPath.parent_path();
    }

    const auto Reconv = [&](const uint32_t _uOrder)
    {
        if (std::filesystem::is_directory(InputPath))
        {
            for (const auto& Entry : std::filesystem::directory_iterator(InputPath))
            {
                if (Entry.is_directory() == false && Entry.path().extension() == ".dot")
                {
                    dot2ll(Entry.path().string(), _uOrder, bReconv, OutputPath, bVirtualFront, bCloseSuccAfterAddNode, sCustomOrder);
                }
            }
        }
        else
        {
            dot2ll(InputPath.string(), _uOrder, bReconv, OutputPath, bVirtualFront, bCloseSuccAfterAddNode, sCustomOrder);
        }
    };


    for (uint32_t i = 0u; i < NodeOrdering::NumOf; ++i)
    {
        const auto kType = NodeOrdering::Type((1 << i));
        if ((kOrder & kType) == kType)
        {            
            if (kType == NodeOrdering::Custom && sCustomOrder.empty())
                continue;

            Reconv(i);
        }
    }

    return 0;
}
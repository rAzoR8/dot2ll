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
    L"PostOrder",
    L"ReversePostOrder",
    L"DominanceRegion",
    L"Custom"
};


std::vector<InstrId> dot2ll(const std::string& _sDotFile, const uint32_t _uOderIndex, const bool _bReconv, const std::filesystem::path& _sOutPath, const bool _bPutVirtualFront, const std::string& _sCustomOrder)
{
    const NodeOrdering::OrderType _kOrder{ 1u << _uOderIndex };

    DotGraph dotin = DotParser::ParseFromFile(_sDotFile);

    const size_t uUserNodes = dotin.GetNodes().size();

    if (uUserNodes == 0u)
    {
        HERROR("Failed to parse %s", WCSTR(_sDotFile));
        return {};
    }

    Function func = Dot2CFG::Convert(dotin);

    if (func.EnforceUniqueEntryPoint() == false || func.EnforceUniqueExitPoint() == false)
        return {};

    const bool bInputReconverging = CheckReconvergence::IsReconverging(func);

    HLOG("Processing %s '%s' [Order: %s Reconv: %s]", WCSTR(_sDotFile), WCSTR(dotin.GetName()),
        _kOrder == NodeOrdering::Order_Custom ? WCSTR(_sCustomOrder) : WCSTR(OrderNames[_uOderIndex]), bInputReconverging ? L"true" : L"false");

    std::string sOutName = dotin.GetName();
    std::vector<InstrId> BBOrder;

    if (_bReconv)
    {
        sOutName += "_reconv";

        NodeOrder InputOrdering;
        
        switch (_kOrder)
        {
        case NodeOrdering::Order_DepthFirst:
            InputOrdering = NodeOrdering::DepthFirst(func.GetEntryBlock());
            sOutName += "_df";
            break;
        case NodeOrdering::Order_BreadthFirst:
            InputOrdering = NodeOrdering::BreadthFirst(func.GetEntryBlock(), false);
            sOutName += "_bf";
            break;
        case NodeOrdering::Order_BreadthFirstDom:
            InputOrdering = NodeOrdering::BreadthFirst(func.GetEntryBlock(), true);
            sOutName += "_bfd";
            break;
        case NodeOrdering::Order_PostOrder:
            InputOrdering = NodeOrdering::PostOrderTraversal(func.GetEntryBlock(), false);
            sOutName += "_pot";
            break;
        case NodeOrdering::Order_ReversePostOrder:
            InputOrdering = NodeOrdering::PostOrderTraversal(func.GetEntryBlock(), true);
            sOutName += "_rpot";
            break;
        case NodeOrdering::Order_DominanceRegion:
            InputOrdering = NodeOrdering::DominanceRegion(func.GetEntryBlock());
            sOutName += "_domreg";
            break;
        case NodeOrdering::Order_Custom:
            InputOrdering = NodeOrdering::Custom(func.GetCFG(), _sCustomOrder);
            sOutName += "_custom";
            break;
        case NodeOrdering::Order_DepthFirstDom:
        default:
            InputOrdering = NodeOrdering::DepthFirstPostDom(func.GetEntryBlock(), func.GetExitBlock());
            sOutName += "_dfd";
            break;
        }

        if (InputOrdering.size() != uUserNodes)
        {
            HFATALD("Ordering is not a valid traversal of the input CFG!");
            return {};
        }

        for (BasicBlock* pBB : InputOrdering)
        {
            BBOrder.push_back(pBB->GetIdentifier());
        }

        // only execute if nodes in ordering are not reconverging already
        bool bChangedCFG = !bInputReconverging ? NodeOrdering::PrepareOrdering(InputOrdering, _bPutVirtualFront, true) : false;

        // reconverge using InputOrdering
        OpenTree OT(true, _sOutPath.string() + "/");
        bChangedCFG = OT.Process(InputOrdering);

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
    }

    std::ofstream ll(_sOutPath / (sOutName + ".ll"));

    if (ll.is_open())
    {
        func.Finalize();

        InstructionSetLLVMAMD isa;
        isa.SerializeListing(func, ll);

        ll.close();
    }

    return BBOrder;
}

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    uint32_t kOrder = NodeOrdering::Order_None;
    hlx::Logger::Instance()->WriteToStream(&std::wcout);

    std::filesystem::path InputPath;
    std::filesystem::path OutputPath;
    std::string sCustomOrder;

    bool bReconv = false;
    bool bVirtualFront = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string token(argv[i]);

        if (token == "-reconverge" || token == "-r")
        {
            bReconv = true;
        }
        else if (token == "-depthfirst" || token == "-df")
        {
            kOrder |= NodeOrdering::Order_DepthFirst;
        }
        else if (token == "-depthfirstdom" || token == "-dfd")
        {
            kOrder |= NodeOrdering::Order_DepthFirstDom;
        }
        else if (token == "-breadthfirst" || token == "-bf")
        {
            kOrder |= NodeOrdering::Order_BreadthFirst;
        }
        else if (token == "-breadthfirstdom" || token == "-bfd")
        {
            kOrder |= NodeOrdering::Order_BreadthFirstDom;
        }
        else if (token == "-postorder" || token == "-pot")
        {
            kOrder |= NodeOrdering::Order_PostOrder;
        }
        else if (token == "-reversepostorder" || token == "-rpot")
        {
            kOrder |= NodeOrdering::Order_ReversePostOrder;
        }
        else if (token == "-dominanceregion" || token == "-domreg")
        {
            kOrder |= NodeOrdering::Order_DominanceRegion;
        }
        else if (token == "-all")
        {
            kOrder = NodeOrdering::Order_All;
        }
        else if (token == "-custom" && (i + 1) < argc)
        {
            kOrder |= NodeOrdering::Order_Custom;
            sCustomOrder = argv[++i];
        }
        else if (token == "-virtualfront")
        {
            bVirtualFront = true;
        }
        else if (token == "-out" && (i + 1) < argc)
        {
            OutputPath = argv[++i];
        }
        else if (token == "-in" && (i + 1) < argc)
        {
            InputPath = argv[++i];
        }
        else if(i == 1u)
        {
            InputPath = token;
        }
    }

    if (OutputPath.empty())
    {
        OutputPath = std::filesystem::is_directory(InputPath) ? InputPath : InputPath.parent_path();
    }

    if (kOrder == 0u)
    {
        HWARNING("No input ordering specified, defaulting to DFPD");
        kOrder = NodeOrdering::Order_DepthFirstDom;
    }

    const auto Reconv = [&](const uint32_t _uOrder)
    {
        if (std::filesystem::is_directory(InputPath))
        {
            for (const auto& Entry : std::filesystem::directory_iterator(InputPath))
            {
                if (Entry.is_directory() == false && Entry.path().extension() == ".dot")
                {
                    dot2ll(Entry.path().string(), _uOrder, bReconv, OutputPath, bVirtualFront, sCustomOrder);
                }
            }
        }
        else
        {
            dot2ll(InputPath.string(), _uOrder, bReconv, OutputPath, bVirtualFront, sCustomOrder);
        }
    };

#if 1
    for (const auto& Entry : std::filesystem::directory_iterator(InputPath))
    {
        if (Entry.is_directory() == false && Entry.path().extension() == ".dot")
        {
            auto dfd = dot2ll(Entry.path().string(), 1, bReconv, OutputPath, bVirtualFront, sCustomOrder);
            auto domreg = dot2ll(Entry.path().string(), 6, bReconv, OutputPath, bVirtualFront, sCustomOrder);
            if (dfd != domreg)
            {
                HWARNING("Orderings dont match for %s", WCSTR(Entry.path().filename()));
            }
        }
    }
#else
    for (uint32_t i = 0u; i < NodeOrdering::Order_NumOf; ++i)
    {
        const auto kType = NodeOrdering::OrderType((1 << i));
        if ((kOrder & kType) == kType)
        {            
            if (kType == NodeOrdering::Order_Custom && sCustomOrder.empty())
                continue;

            Reconv(i);
        }
    }
#endif

    return 0;
}
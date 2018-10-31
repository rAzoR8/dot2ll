#include "DotParser.h"
#include "DotWriter.h"
#include "Dot2CFG.h"
#include "CFG2Dot.h"
#include "OpenTree.h"
#include "InstructionSetLLVMAMD.h"
#include "CheckReconvergence.h"

int main(int argc, char* argv[])
{
    hlx::Logger::Instance()->WriteToStream(&std::wcout);

    std::string sInputFile;

    if (argc < 2)
        return 1;

    NodeOrdering::Type kOrder = NodeOrdering::DepthFirstDom;

    bool bReconv = false;
    //bool bCheck = false;

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
        //else if (token == "-c")
        //{
        //    bCheck = false;
        //}
        else
        {
            sInputFile = token;
        }
    }

    DotGraph dotin = DotParser::ParseFromFile(sInputFile);

    if (dotin.GetNodes().empty() == false)
    {
        Function func = Dot2CFG::Convert(dotin);

        const bool bReconverges = CheckReconvergence::IsReconverging(func, !bReconv, true);
        HLOG("Input Function %s is reconverging: %s", WCSTR(func.GetName()), bReconverges ? L"true" : L"false");

        std::string sOutLL = dotin.GetName();

        if (bReconv)
        {
            sOutLL += "_reconv";

            func.EnforceUniqueExitPoint();

            NodeOrder InputOrdering;

            switch (kOrder)
            {
            case NodeOrdering::DepthFirst:
                InputOrdering = NodeOrdering::ComputeDepthFirst(func.GetEntryBlock());
                break;
            case NodeOrdering::BreadthFirst:
                InputOrdering = NodeOrdering::ComputeBreadthFirst(func.GetEntryBlock());
                break;
            case NodeOrdering::DepthFirstDom:
            default:
                InputOrdering = NodeOrdering::ComputePaper(func.GetEntryBlock(), func.GetExitBlock());
                break;
            }

            // reconverge using InputOrdering
            OpenTree OT;
            OT.Process(InputOrdering);

            func.Finalize();

            HASSERT(CheckReconvergence::IsReconverging(func, true), "Function %s is NOT reconverging!", WCSTR(func.GetName()));

            const std::string sOutDot = dotin.GetName() + "_reconv.dot";
            std::ofstream dotout(sOutDot);

            if (dotout.is_open())
            {
                DotWriter::WriteToStream(CFG2Dot::Convert(func.GetCFG(), func.GetName()), dotout);

                dotout.close();
            }
        }

        sOutLL += ".ll";
        std::ofstream ll(sOutLL.c_str());

        if (ll.is_open())
        {
            func.Finalize();
            
            InstructionSetLLVMAMD isa;
            isa.SerializeListing(func, ll);

            ll.close();
        }
    }

    return 0;
}
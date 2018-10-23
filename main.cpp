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

    bool bReconv = false;
    //bool bCheck = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string token(argv[i]);

        if (token == "-r")
        {
            bReconv = true;
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

        const bool bReconverges = CheckReconvergence::IsReconverging(func, true);
        HLOG("Input Function %s is reconverging: %s", WCSTR(func.GetName()), bReconverges ? L"true" : L"false");

        const std::string sOutLL = dotin.GetName() + ".ll";

        if (bReconv)
        {
            func.EnforceUniqueExitPoint();

            //NodeOrder InputOrdering = NodeOrdering::ComputeDepthFirst(func.GetEntryBlock());
            NodeOrder InputOrdering = NodeOrdering::ComputeBreadthFirst(func.GetEntryBlock());
            //NodeOrder InputOrdering = NodeOrdering::ComputePaper(func.GetEntryBlock(), func.GetExitBlock());

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
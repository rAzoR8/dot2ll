#include "DotParser.h"
#include "DotWriter.h"
#include "Dot2CFG.h"
#include "CFG2Dot.h"
#include "OpenTree.h"
#include "InstructionSetLLVMAMD.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;
    
    const bool bReconv = argc > 2 && std::string(argv[2]) == "-r";

    DotGraph dotin = DotParser::ParseFromFile(argv[1]);

    if (dotin.GetNodes().empty() == false)
    {
        Function func = Dot2CFG::Convert(dotin);

        const std::string sOutLL = dotin.GetName() + ".ll";

        if (bReconv)
        {
            //NodeOrder InputOrdering = NodeOrdering::ComputeDepthFirst(func.GetEntryBlock());
            //NodeOrder InputOrdering = NodeOrdering::ComputeBreadthFirst(func.GetEntryBlock());
            NodeOrder InputOrdering = NodeOrdering::ComputePaper(func.GetEntryBlock(), func.GetExitBlock());

            // reconverge using InputOrdering
            OpenTree OT;
            OT.Process(InputOrdering);

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
            InstructionSetLLVMAMD isa;
            isa.SerializeListing(func, ll);

            ll.close();
        }
    }

    return 0;
}
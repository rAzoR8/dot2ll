#include "DotParser.h"
#include "Dot2CFG.h"
#include "OpenTree.h"
#include "InstructionSetLLVMAMD.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    DotGraph dotgraph = DotParser::ParseFromFile(argv[1]);

    if (dotgraph.GetNodes().empty() == false)
    {
        const std::string sOutLL = dotgraph.GetName() + ".ll";
        std::ofstream ll(argc > 2 ? argv[2] : sOutLL.c_str());

        if (ll.is_open())
        {
            Function func = Dot2CFG::Convert(dotgraph);

            NodeOrder InputOrdering = NodeOrdering::ComputeDepthFirst(func.GetEntryBlock());

            // reconverge using InputOrdering
            OpenTree OT;
            OT.Process(InputOrdering);

            InstructionSetLLVMAMD isa;
            isa.SerializeListing(func, ll);

            ll.close();
        }
    }

    return 0;
}
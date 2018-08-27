#include "DotParser.h"
#include "Dot2CFG.h"
#include "CFG2LL.h"

int main(int argc, char* argv[])
{
    if (argc < 2)
        return 1;

    DotGraph dotgraph = DotParser::ParseFromFile(argv[1]);

    if (dotgraph.GetNodes().empty() == false)
    {
        DefaultCFG cfg = Dot2CFG::Convert(dotgraph);
        const std::string sFuncDef = CFG2LL::Generate(cfg);

        const std::string sOutLL = dotgraph.GetName() + ".ll";
        std::ofstream ll(argc > 2 ? argv[2] : sOutLL.c_str());

        if (ll.is_open())
        {
            CFG2LL::WriteToStdStream(cfg, sFuncDef, ll);
            ll.close();
        }
    }

    return 0;
}
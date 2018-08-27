#pragma once

#include "ControlFlowGraph.h"
#include "CFGUtils.h"

class LowerReconvCFG
{
public:
    LowerReconvCFG() {};
    ~LowerReconvCFG() {};

    template<class TInstruction, class TNodeAttrib, class TEdgeAttrib>
    static bool LowerToWaveLevel(TControlFlowGraph<TInstruction, TNodeAttrib, TEdgeAttrib>& _Graph);

private:

};

template<class TInstruction, class TNodeAttrib, class TEdgeAttrib>
inline bool LowerReconvCFG::LowerToWaveLevel(TControlFlowGraph<TInstruction, TNodeAttrib, TEdgeAttrib>& _Graph)
{
    return false;
}

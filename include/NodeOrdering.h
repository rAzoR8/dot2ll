#pragma once

#include <list>

#include "BasicBlock.h"

// forward decls:
class ControlFlowGraph;

using NodeOrder = std::list<BasicBlock*>;

class NodeOrdering
{
public:
    enum Type : uint32_t
    {
        None = 0,
        DepthFirst = 1 << 0,
        DepthFirstDom = 1 << 1,
        BreadthFirst = 1 << 2,
        BreadthFirstDom = 1 << 3,
        RPOT = 1 << 4,
        Custom = 1 << 5,
        NumOf = 6,
        All = (Custom << 1) - 1        
    };

    NodeOrdering() {};
    ~NodeOrdering() {};

    // _sCustomOrdering list of comma separated block names: A,B,C,D
    static NodeOrder ComputeCustomOrder(ControlFlowGraph& _CFG, const std::string& _sCustomOrdering);

    static NodeOrder ComputeDepthFirst(BasicBlock* _pRoot, const bool _bExitLast = false);

    static NodeOrder ComputeReversePostorderTraversal(BasicBlock* _pExit);

    static NodeOrder ComputeBreadthFirst(BasicBlock* _pRoot, const bool _bCheckAncestors = false);

    static NodeOrder ComputePaper(BasicBlock* _pRoot, BasicBlock* _pExit);

    // convert divergent nodes with 2 backwards edges to 1 backwards and two foward edges
    // returns true if virtual nodes were inserted
    static bool PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront = false);

private:

};

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
        DepthFirst = 0,
        DepthFirstDom,
        BreadthFirst,
        BreadthFirstDom,
        All,
        Custom
    };

    NodeOrdering() {};
    ~NodeOrdering() {};

    // _sCustomOrdering list of comma separated block names: A,B,C,D
    static NodeOrder ComputeCustomOrder(ControlFlowGraph& _CFG, const std::string& _sCustomOrdering);

    static NodeOrder ComputeDepthFirst(BasicBlock* _pRoot);

    static NodeOrder ComputeBreadthFirst(BasicBlock* _pRoot, const bool _bCheckAncestors = false);

    static NodeOrder ComputePaper(BasicBlock* _pRoot, BasicBlock* _pExit);

    // convert divergent nodes with 2 backwards edges to 1 backwards and two foward edges
    static void PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront = false);

private:

};

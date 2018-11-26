#pragma once

#include <list>

#include "BasicBlock.h"

// forward decls:
class ControlFlowGraph;

using NodeOrder = std::list<BasicBlock*>;

class NodeOrdering
{
public:
    enum OrderType : uint32_t
    {
        Order_None = 0,
        Order_DepthFirst = 1 << 0,
        Order_DepthFirstDom = 1 << 1,
        Order_BreadthFirst = 1 << 2,
        Order_BreadthFirstDom = 1 << 3,
        Order_PostOrder = 1 << 4,
        Order_ReversePostOrder = 1 << 5,
        Order_DominanceRegion = 1 << 6,
        Order_Custom = 1 << 7,
        Order_NumOf = 8,
        Order_All = (Order_Custom << 1) - 1
    };

    NodeOrdering() {};
    ~NodeOrdering() {};

    // _sCustomOrdering list of comma separated block names: A,B,C,D
    static NodeOrder Custom(ControlFlowGraph& _CFG, const std::string& _sCustomOrdering);

    static NodeOrder DepthFirst(BasicBlock* _pRoot, const bool _bExitLast = false);

    static NodeOrder DepthFirstPostDom(BasicBlock* _pRoot, BasicBlock* _pExit);

    static NodeOrder BreadthFirst(BasicBlock* _pRoot, const bool _bCheckAncestors = false);

    static NodeOrder PostOrderTraversal(BasicBlock* _pExit, const bool _bReverse = false);

    static NodeOrder DominanceRegion(BasicBlock* _pRoot);

    // convert divergent nodes with 2 backwards edges to 1 backwards and two foward edges
    // returns true if virtual nodes were inserted
    static bool PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront = false, const bool _bExitLast = true);

private:

};

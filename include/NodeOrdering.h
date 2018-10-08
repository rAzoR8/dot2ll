#pragma once

#include "BasicBlock.h"
#include <unordered_set>
#include <list>

using NodeOrder = std::list<BasicBlock*>;

// forward decl
class DominatorTree;

class NodeOrdering
{
public:
    NodeOrdering() {};
    ~NodeOrdering() {};

    static NodeOrder ComputeDepthFirst(BasicBlock* _pRoot);

    static NodeOrder ComputeBreadthFirst(BasicBlock* _pRoot);

    static NodeOrder ComputePaper(BasicBlock* _pRoot);

    // convert divergent nodes with 2 backwards edges to 1 backwards and two foward edges
    static void PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront = false);

private:
    static void ComputePaper(BasicBlock* _pBB, std::unordered_set<BasicBlock*>& _Visited, const DominatorTree& _PDT, NodeOrder& _Order);

};

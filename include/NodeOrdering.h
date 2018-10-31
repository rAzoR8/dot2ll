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
    enum Type
    {
        DepthFirst = 0,
        BreadthFirst = 1,
        DepthFirstDom = 2,
    };

    NodeOrdering() {};
    ~NodeOrdering() {};

    static NodeOrder ComputeDepthFirst(BasicBlock* _pRoot);

    static NodeOrder ComputeBreadthFirst(BasicBlock* _pRoot);

    static NodeOrder ComputePaper(BasicBlock* _pRoot, BasicBlock* _pExit);

    // convert divergent nodes with 2 backwards edges to 1 backwards and two foward edges
    static void PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront = false);

private:

};

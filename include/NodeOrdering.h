#pragma once

#include "BasicBlock.h"
#include <list>

using NodeOrder = std::list<BasicBlock*>;

class NodeOdering
{
public:
    NodeOdering() {};
    ~NodeOdering() {};

    static NodeOrder ComputeDepthFirst(BasicBlock* _pRoot);

    // convert divergent nodes with 2 backwards edges to 1 backwards and two foward edges
    static void PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront = false);

private:

};

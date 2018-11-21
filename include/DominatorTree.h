#pragma once

#include "BasicBlock.h"
#include <unordered_map>

class DominatorTree
{
public:
    // _pRoot is the source of a dominator tree
    // and the sink for a post-dominator tree
    DominatorTree(const BasicBlock* _pRoot = nullptr, const bool _PostDom = false);
    DominatorTree(DominatorTree&& _Other);
    DominatorTree& operator=(DominatorTree&& _Other);

    ~DominatorTree() {};

    bool Dominates(const BasicBlock* _pDominator, const BasicBlock* _pBlock) const;

private:
    // dominator -> dominated nodes
    std::unordered_multimap<const BasicBlock*, const BasicBlock*> m_DominatorMap;
};
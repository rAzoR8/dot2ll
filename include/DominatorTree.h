#pragma once

#include "BasicBlock.h"
#include <unordered_map>

class DominatorTree
{
public:
    DominatorTree(const BasicBlock* _pRoot, const bool _PostDom = false);
    ~DominatorTree() {};

    bool Dominates(const BasicBlock* _pDominator, const BasicBlock* _pBlock) const;

private:
    // dominator -> dominated nodes
    std::unordered_multimap<const BasicBlock*, const BasicBlock*> m_DominatorMap;
};
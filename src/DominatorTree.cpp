#include "DominatorTree.h"
#include "CFGUtils.h"

DominatorTree::DominatorTree(const BasicBlock* _pRoot, const bool _bPostDom)
{
    std::unordered_set<const BasicBlock*> Set;
    std::vector<const BasicBlock*> BBs;

    CFGUtils::DepthFirst(_pRoot, BBs, Set, !_bPostDom);

    for (auto it = BBs.begin(), end = BBs.end(); it != end; ++it)
    {
        const BasicBlock* pDom = *it;

        for (auto it2 = BBs.begin() + 1; it2 != end; ++it2)
        {
            const BasicBlock* pBlock = *it2;
            if (CFGUtils::IsReachable(pBlock, _pRoot, Set, !_bPostDom, pDom) == false)
            {
                m_DominatorMap.insert({ pDom, pBlock });
            }

            Set.clear();
        }
    }
}

bool DominatorTree::Dominates(const BasicBlock* _pDominator, const BasicBlock* _pBlock) const
{
    if (_pDominator == _pBlock)
        return true;

    auto range = m_DominatorMap.equal_range(_pDominator);

    for (auto it = range.first; it != range.second; ++it)
    {
        if (it->second == _pBlock)
            return true;
    }

    return false;
}

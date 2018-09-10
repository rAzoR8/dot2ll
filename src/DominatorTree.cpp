#include "DominatorTree.h"

DominatorTree::DominatorTree(const BasicBlock* _pRoot, const bool _bPostDom) :
    m_Root(nullptr, _pRoot)
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

            }

            Set.clear();
        }
    }
}

bool DominatorTree::Dominates(const BasicBlock* _pDominator, const BasicBlock* _pBlock) const
{
    if (_pDominator == _pBlock)
        return true;

    auto it = m_NodeMap.find(_pBlock);
    if (it != m_NodeMap.end())
    {
        const DominatorTreeNode* pNode = it->second;
        do
        {
            if (pNode->GetBasicBlock() == _pDominator)
                return true;

            pNode = pNode->GetParent();
        } while (pNode != nullptr);
    }

    return false;
}

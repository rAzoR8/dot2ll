#include "DominatorTree.h"
#include "CFGUtils.h"

DominatorTree::DominatorTree(const BasicBlock* _pRoot, const bool _bPostDom)
{
    if (_pRoot != nullptr)
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
                Set.clear();

                if (pDom == _pRoot || CFGUtils::IsReachable(pBlock, _pRoot, Set, !_bPostDom, pDom) == false)
                {
                    m_DominatorMap.insert({ pDom, pBlock });           
                }
            }
        }

        m_Nodes.reserve(BBs.size());

        for (const BasicBlock* pBB : BBs)
        {
            m_NodeMap[pBB] = &m_Nodes.emplace_back(pBB);
        }

        m_pRoot = &m_Nodes.front();

        for (const auto&[Dom, Sub] : m_DominatorMap)
        {
            DominatorTreeNode* pDom = m_NodeMap[Dom];
            DominatorTreeNode* pSub = m_NodeMap[Sub];

            pDom->Append(*this, pSub);
        }
    }
}

DominatorTree::DominatorTree(DominatorTree&& _Other) :
    m_DominatorMap(std::move(_Other.m_DominatorMap))
{
    m_Nodes.reserve(_Other.m_Nodes.size());
    
    for (auto& Node : _Other.m_Nodes)
    {
        m_NodeMap[Node.GetBasicBlock()] = &m_Nodes.emplace_back(Node.GetBasicBlock());
    }

    m_pRoot = &m_Nodes.front();

    for (const auto&[Dom, Sub] : m_DominatorMap)
    {
        DominatorTreeNode* pDom = m_NodeMap[Dom];
        DominatorTreeNode* pSub = m_NodeMap[Sub];

        pDom->Append(*this, pSub);
    }

    //_Other.m_NodeMap.clear();
    //_Other.m_Nodes.clear();
}

DominatorTree& DominatorTree::operator=(DominatorTree && _Other)
{
    m_DominatorMap = std::move(_Other.m_DominatorMap);

    m_NodeMap.clear();
    m_Nodes.clear();

    m_Nodes.reserve(_Other.m_Nodes.size());

    for (auto& Node : _Other.m_Nodes)
    {
        m_NodeMap[Node.GetBasicBlock()] = &m_Nodes.emplace_back(Node.GetBasicBlock());
    }

    m_pRoot = &m_Nodes.front();

    for (const auto&[Dom, Sub] : m_DominatorMap)
    {
        DominatorTreeNode* pDom = m_NodeMap[Dom];
        DominatorTreeNode* pSub = m_NodeMap[Sub];

        pDom->Append(*this, pSub);
    }

    return *this;
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

void DominatorTreeNode::Append(const DominatorTree& DT, DominatorTreeNode* _pSub)
{
    if (this == _pSub)
        return;

    for (DominatorTreeNode* pChild : m_Children)
    {
        if (DT.Dominates(pChild->GetBasicBlock(), _pSub->GetBasicBlock()))
        {
            pChild->m_bEntryAttached |= _pSub->m_pBasicBlock->IsSource();
            pChild->m_bExitAttached |= _pSub->m_pBasicBlock->IsSink();

            pChild->Append(DT, _pSub);
            return;
        }
    }

    _pSub->m_bEntryAttached |= _pSub->m_pBasicBlock->IsSource();
    _pSub->m_bExitAttached |= _pSub->m_pBasicBlock->IsSink();
    _pSub->m_pParent = this;
    m_Children.push_back(_pSub);

    //if (_pSub->IsSink())
    //{
    //    do
    //    {
    //        pSubNode->m_bExitAttached = true;
    //        pSubNode = pSubNode->m_pParent;
    //    } while (pSubNode != nullptr);
    //}

    //if (_pSub->IsSource())
    //{
    //    do
    //    {
    //        pSubNode->m_bEntryAttached = true;
    //        pSubNode = pSubNode->m_pParent;
    //    } while (pSubNode != nullptr);
    //}
}

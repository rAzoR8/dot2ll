#include "DominatorTree.h"
#include "CFGUtils.h"

DominatorTree::DominatorTree(const BasicBlock* _pRoot, const bool _bPostDom)
{
    if (_pRoot != nullptr)
    {
        std::unordered_set<const BasicBlock*> Set;
        std::vector<const BasicBlock*> BBs;

        CFGUtils::DepthFirst(_pRoot, BBs, Set, !_bPostDom);

        m_Nodes.reserve(BBs.size());
        m_pRoot = &m_Nodes.emplace_back(nullptr, _pRoot);
        m_NodeMap[_pRoot] = m_pRoot;

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
                    Append(pDom, pBlock);
                }
            }
        }
    }
}

DominatorTree::DominatorTree(DominatorTree&& _Other) :
    m_DominatorMap(std::move(_Other.m_DominatorMap))
{
    m_Nodes.reserve(_Other.m_Nodes.size());
    
    m_pRoot = &m_Nodes.emplace_back(nullptr, _Other.m_pRoot->m_pBasicBlock);
    m_NodeMap[m_pRoot->m_pBasicBlock] = m_pRoot;

    _Other.m_NodeMap.clear();
    _Other.m_Nodes.clear();

    for (const auto&[dom, sub] : m_DominatorMap)
    {
        Append(dom, sub);
    }
}

DominatorTree& DominatorTree::operator=(DominatorTree && _Other)
{
    m_DominatorMap = std::move(_Other.m_DominatorMap);

    m_NodeMap.clear();
    m_Nodes.clear();

    m_Nodes.reserve(_Other.m_Nodes.size());

    m_pRoot = &m_Nodes.emplace_back(nullptr, _Other.m_pRoot->m_pBasicBlock);
    m_NodeMap[m_pRoot->m_pBasicBlock] = m_pRoot;

    _Other.m_NodeMap.clear();
    _Other.m_Nodes.clear();

    for (const auto&[dom, sub] : m_DominatorMap)
    {
        Append(dom, sub);
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

void DominatorTree::Append(const BasicBlock* _pDom, const BasicBlock* _pSub)
{
    if (_pDom == _pSub)
        return;

    DominatorTreeNode* pDomNode = nullptr;
    auto domit = m_NodeMap.find(_pDom);
    if (domit == m_NodeMap.end())
    {
        pDomNode = &m_Nodes.emplace_back(m_pRoot, _pDom);
        m_NodeMap[_pDom] = pDomNode;
    }
    else
    {
        pDomNode = domit->second;
    }

    DominatorTreeNode* pSubNode = nullptr;
    auto subit = m_NodeMap.find(_pSub);
    if (subit == m_NodeMap.end())
    {
        pSubNode = &m_Nodes.emplace_back(pDomNode, _pSub);
        m_NodeMap[_pSub] = pSubNode;
    }
    else
    {
        pSubNode = subit->second;
    }

    pDomNode->m_Children.push_back(pSubNode);
}

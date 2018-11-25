#pragma once

#include "BasicBlock.h"
#include <unordered_map>

class DominatorTreeNode
{
    friend class DominatorTree;
public:
    DominatorTreeNode(DominatorTreeNode* _pParent = nullptr, const BasicBlock* _pBB = nullptr) :
        m_sName(_pBB->GetName()), m_pParent(_pParent), m_pBasicBlock(_pBB) {}

    const std::vector<DominatorTreeNode*>& GetChildren() const { return m_Children; }

private:
    const std::string m_sName;
    const BasicBlock* m_pBasicBlock;
    DominatorTreeNode* m_pParent;

    std::vector<DominatorTreeNode*> m_Children;
};

class DominatorTree
{
public:
    // _pRoot is the source of a dominator tree
    // and the sink for a post-dominator tree
    DominatorTree(const BasicBlock* _pRoot = nullptr, const bool _PostDom = false);
    DominatorTree(DominatorTree&& _Other);
    DominatorTree(const DominatorTree& _Other) = delete;

    DominatorTree& operator=(DominatorTree&& _Other);
    DominatorTree& operator=(const DominatorTree& _Other) = delete;

    ~DominatorTree() {};

    bool Dominates(const BasicBlock* _pDominator, const BasicBlock* _pBlock) const;

    DominatorTreeNode* GetRootNode() const { return m_pRoot; };

private:
    void Append(const BasicBlock* _pDom, const BasicBlock* _pSub);

private:
    // dominator -> dominated nodes (for constant lookup)
    std::unordered_multimap<const BasicBlock*, const BasicBlock*> m_DominatorMap;
    std::unordered_map<const BasicBlock*, DominatorTreeNode*> m_NodeMap;

    std::vector<DominatorTreeNode> m_Nodes;
    DominatorTreeNode* m_pRoot = nullptr;
};
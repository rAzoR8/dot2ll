#pragma once

#include "BasicBlock.h"
#include <unordered_map>

class DominatorTreeNode
{
    friend class DominatorTree;
public:
    DominatorTreeNode(const BasicBlock* _pBB = nullptr) :
        m_sName(_pBB->GetName()), m_pBasicBlock(_pBB) {}

    const std::vector<DominatorTreeNode*>& GetChildren() const { return m_Children; }

    const BasicBlock* GetBasicBlock() const { return m_pBasicBlock; }
    BasicBlock* GetBasicBlock() { return const_cast<BasicBlock*>(m_pBasicBlock); }

    const bool ExitAttached() const { return m_bExitAttached; }
    const bool EntryAttached() const { return m_bEntryAttached; }

private:
    void Append(const DominatorTree& DT, DominatorTreeNode* _pSub);

private:
    const std::string m_sName;
    const BasicBlock* m_pBasicBlock;
    DominatorTreeNode* m_pParent = nullptr;
    bool m_bExitAttached = false;
    bool m_bEntryAttached = false;

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
    // dominator -> dominated nodes (for constant lookup)
    std::unordered_multimap<const BasicBlock*, const BasicBlock*> m_DominatorMap;
    std::unordered_map<const BasicBlock*, DominatorTreeNode*> m_NodeMap;

    std::vector<DominatorTreeNode> m_Nodes;
    DominatorTreeNode* m_pRoot = nullptr;
};
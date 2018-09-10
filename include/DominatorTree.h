#pragma once

#include "BasicBlock.h"
#include <unordered_map>
#include "CFGUtils.h"

class DominatorTreeNode
{
public:
    DominatorTreeNode(const DominatorTreeNode* _pParent = nullptr, const BasicBlock* _pBasicBlock = nullptr) :
       m_pParent(_pParent), m_pBasicBlock(_pBasicBlock){};
    ~DominatorTreeNode() {};

    const DominatorTreeNode* GetParent() const { return m_pParent; }
    const BasicBlock* GetBasicBlock() const { return m_pBasicBlock; }
private:
    const DominatorTreeNode* m_pParent;
    const BasicBlock* m_pBasicBlock;
    std::vector<DominatorTreeNode> m_Children;
};

class DominatorTree
{
public:
    DominatorTree(const BasicBlock* _pRoot, const bool _PostDom = false);
    ~DominatorTree() {};

    bool Dominates(const BasicBlock* _pDominator, const BasicBlock* _pBlock) const;

private:
    std::unordered_map<const BasicBlock*, const DominatorTreeNode*> m_NodeMap;
    DominatorTreeNode m_Root;
};
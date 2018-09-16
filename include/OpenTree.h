#pragma once

#include "NodeOrdering.h"
#include <unordered_map>

struct OpenTreeNode
{
    OpenTreeNode(BasicBlock* _pBB = nullptr) : 
        pBB(_pBB)
    {
        if (_pBB != nullptr)
        {
            Incomping = _pBB->GetPredecessors();
            Outgoing = _pBB->GetSuccesors();
        }
    }

    // is non-uniform (divergent) and one of the outgoing edges has already been closed
    bool Armed() const { return pBB->IsDivergent() && pBB->GetSuccesors().size() == 2u && Outgoing.size() == 1u; }
    bool Visited() const { return Incomping.empty() && Outgoing.empty(); }
    bool InOT() const { return pParent != nullptr; }

    std::vector<OpenTreeNode*> Children;

    OpenTreeNode* pParent = nullptr;
    BasicBlock* pBB = nullptr;

    // open edges
    BasicBlock::Vec Incomping;
    BasicBlock::Vec Outgoing;
};

class OpenTree
{
public:
    OpenTree() {};
    ~OpenTree() {};

    void Process(const NodeOrder& _Ordering);
    
private:
    void Prepare(NodeOrder& _Ordering);

    void AddNode(BasicBlock* _pBB);

    std::vector<OpenTreeNode*> GetArmedPredecessors(BasicBlock* _pBB) const;

    std::vector<OpenTreeNode*> GetVistedSuccessors(BasicBlock* _pBB) const;

private:
    std::vector<OpenTreeNode> m_Nodes;
    OpenTreeNode* m_pRoot = nullptr; // virtual root
    std::unordered_map<BasicBlock*, OpenTreeNode*> m_BBToNode;
};

#pragma once

#include "NodeOrdering.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

struct OpenTreeNode
{
    OpenTreeNode(BasicBlock* _pBB = nullptr) : 
        pBB(_pBB)
    {
        if (_pBB != nullptr)
        {
            Incoming = _pBB->GetPredecessors();
            Outgoing = _pBB->GetSuccesors();
        }
    }

    // is non-uniform (divergent) and one of the outgoing edges has already been closed
    bool Armed() const { return pBB->IsDivergent() && pBB->GetSuccesors().size() == 2u && Outgoing.size() == 1u; }
    bool Visited() const { return Incoming.empty() && Outgoing.empty(); }
    bool InOT() const { return pParent != nullptr; }

    void Close(BasicBlock* _OpenEdge, const bool _bRemoveClosed = false);

    std::vector<OpenTreeNode*> Children;

    OpenTreeNode* pParent = nullptr;
    BasicBlock* pBB = nullptr;

    // open edges
    BasicBlock::Vec Incoming;
    BasicBlock::Vec Outgoing;
};

class OpenSubTreeUnion
{
public:
    OpenSubTreeUnion(const std::vector<OpenTreeNode*> _Roots);

    const std::unordered_set<OpenTreeNode*>& GetNodes() const { return m_Nodes; }
    const std::unordered_set<OpenTreeNode*>& GetRoots() const { return m_Roots; }

    const bool HasOutgoingNotLeadingTo(BasicBlock* _pBB) const;
    const bool HasMultiRootsOrOutgoing() const;

private:
    std::unordered_set<OpenTreeNode*> m_Nodes;
    std::unordered_set<OpenTreeNode*> m_Roots;
};

class OpenTree
{
    static bool Armed(const OpenTreeNode* pNode) { return pNode->Armed(); };
    static bool Visited(const OpenTreeNode* pNode) { return pNode->Visited(); };

public:
    OpenTree() {};
    ~OpenTree() {};

    void Process(const NodeOrder& _Ordering);

private:
    void Prepare(NodeOrder& _Ordering);

    void AddNode(BasicBlock* _pBB);
    void RemoveNode(OpenTreeNode* _pClosedNode);

    // return lowest ancestor of BB
    OpenTreeNode* InterleavePathsToBB(BasicBlock* _pBB);

    OpenTreeNode* CommonAncestor(BasicBlock* _pBB) const;

    template <class Filter>
    std::vector<OpenTreeNode*> FilterNodes(const std::vector<BasicBlock*>& _BBs, const Filter& _Filter) const;

private:
    std::vector<OpenTreeNode> m_Nodes;
    OpenTreeNode* m_pRoot = nullptr; // virtual root
    std::unordered_map<BasicBlock*, OpenTreeNode*> m_BBToNode;
};

template<class Filter>
inline std::vector<OpenTreeNode*> OpenTree::FilterNodes(const std::vector<BasicBlock*>& _BBs, const Filter & _Filter) const
{
    std::vector<OpenTreeNode*> Nodes;

    for (BasicBlock* pBB : _BBs)
    {
        if (auto it = m_BBToNode.find(pBB); it != m_BBToNode.end())
        {
            if (_Filter(it->second))
            {
                Nodes.push_back(it->second);
            }
        }
    }

    return Nodes;
}
#pragma once

#include "NodeOrdering.h"
#include <unordered_map>
#include <unordered_set>
#include <algorithm>

struct OpenTreeNode
{
    OpenTreeNode(BasicBlock* _pBB = nullptr);

    // is non-uniform (divergent) and one of the outgoing edges has already been closed
    bool Armed() const { return pBB->IsDivergent() && pBB->GetSuccesors().size() == 2u && Outgoing.size() == 1u; }
    bool Visited() const { return bVisited; }
    bool InOT() const { return pParent != nullptr; }
    bool AncestorOf(const OpenTreeNode* _pSuccessor) const;

    // called on predecesssor to close Pred->Succ
    void Close(OpenTreeNode* _Successor, const bool _bRemoveClosed = false);

    std::string sName;

    std::vector<OpenTreeNode*> Children;

    OpenTreeNode* pParent = nullptr;
    BasicBlock* pBB = nullptr;
    bool bVisited = false;
    bool bFlow = false; // is a flowblock

    struct Flow
    {
        BasicBlock* pSource = nullptr; // Original Source S -> Flow -> (True/False)
        Instruction* pCondition = nullptr; // can be null for uncond branches
        BasicBlock* pTarget = nullptr;
        bool bNot = false; // negated conditon
    };

    static void GetOutgoingFlowFromBB(std::vector<Flow>& _OutFlow, BasicBlock* _pSource);

    // open edges
    BasicBlock::Vec Incoming;
    std::vector<Flow> Outgoing;

    // for flow blocks only
    //std::vector<Flow> OutgoingFlow;
    OpenTreeNode* pFirstClosedSuccessor = nullptr; // used only for flow blocks
};

class OpenSubTreeUnion
{
public:
    OpenSubTreeUnion(const std::vector<OpenTreeNode*>& _Roots);

    struct OTNHash
    {
        size_t operator()(const OpenTreeNode* _pNode) const { return _pNode->pBB->GetIdentifier(); }
    };

    const std::unordered_set<OpenTreeNode*, OTNHash>& GetNodes() const { return m_Nodes; }
    const std::unordered_set<OpenTreeNode*>& GetRoots() const { return m_Roots; }

    const bool HasOutgoingNotLeadingTo(BasicBlock* _pBB) const;
    const bool HasMultiRootsOrOutgoing() const;

private:
    // order of nodes does not depend on memory address
    std::unordered_set<OpenTreeNode*, OTNHash> m_Nodes;
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

    void Reroute(OpenSubTreeUnion& _Subtree);

    // return lowest ancestor of BB
    OpenTreeNode* InterleavePathsToBB(BasicBlock* _pBB);

    OpenTreeNode* CommonAncestor(BasicBlock* _pBB) const;

    template <class Filter>
    std::vector<OpenTreeNode*> FilterNodes(const std::vector<BasicBlock*>& _BBs, const Filter& _Filter) const;

private:
    uint32_t m_uNumFlowBlocks = 0u;
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
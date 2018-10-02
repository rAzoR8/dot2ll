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
    OpenTreeNode* GetNode(BasicBlock* _pBB) const;

    void Prepare(NodeOrder& _Ordering);

    void AddNode(BasicBlock* _pBB);

    void Reroute(OpenSubTreeUnion& _Subtree);

    // return lowest ancestor of BB
    OpenTreeNode* InterleavePathsToBB(BasicBlock* _pBB);

    OpenTreeNode* CommonAncestor(BasicBlock* _pBB) const;

    template <class Container, class Filter, class Accessor> // Accessor extracts the OT node from the Container element, Filter processes the OT node and returns true or false
    std::vector<OpenTreeNode*> FilterNodes(const Container& _Container, const Filter& _Filter, const Accessor& _Accessor) const;

    // accessors for FilterNodes()
    OpenTreeNode* operator()(BasicBlock* _pBB) const { return GetNode(_pBB); }
    OpenTreeNode* operator()(const OpenTreeNode::Flow& _Flow) const { return GetNode(_Flow.pTarget); }

private:
    uint32_t m_uNumFlowBlocks = 0u;
    std::vector<OpenTreeNode> m_Nodes;
    OpenTreeNode* m_pRoot = nullptr; // virtual root
    std::unordered_map<BasicBlock*, OpenTreeNode*> m_BBToNode;
};

template<class Container, class Filter, class Accessor>
inline std::vector<OpenTreeNode*> OpenTree::FilterNodes(const Container& _Container, const Filter& _Filter, const Accessor& _Accessor) const
{
    std::vector<OpenTreeNode*> Nodes;

    for (const auto& Elem : _Container)
    {
        if (OpenTreeNode* pNode = _Accessor(Elem); pNode != nullptr && _Filter(pNode))
        {
            Nodes.push_back(pNode);
        }
    }

    return Nodes;
}

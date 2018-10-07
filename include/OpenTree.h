#pragma once

#include "NodeOrdering.h"
#include <unordered_map>
#include <unordered_set>

struct OpenTreeNode
{
    OpenTreeNode(BasicBlock* _pBB = nullptr);

    // is non-uniform (divergent) and one of the outgoing edges has already been closed
    bool Armed() const { return pBB->IsDivergent() && Outgoing.size() == 1u; }
    bool AncestorOf(const OpenTreeNode* _pSuccessor) const;

    // called on predecesssor to close Pred->Succ
    void Close(OpenTreeNode* _Successor, const bool _bRemoveClosed = false);

    std::string sName;

    std::vector<OpenTreeNode*> Children;

    OpenTreeNode* pParent = nullptr;
    BasicBlock* pBB = nullptr;
    bool bVisited = false; // has been added to the OT
    bool bFlow = false; // is a flowblock
    bool bRemoved = false; // for debugging

    struct Flow
    {
        BasicBlock* pSource = nullptr; // Original Source S -> Flow -> (True/False)
        Instruction* pCondition = nullptr; // can be null for uncond branches
        BasicBlock* pTarget = nullptr;
        bool bNot = false; // negated conditon
    };

    static void GetOutgoingFlowFromBB(std::vector<Flow>& _OutFlow, BasicBlock* _pSource);

    // open edges
    std::vector<OpenTreeNode*> Incoming;
    std::vector<Flow> Outgoing;

    // for flow blocks only
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
    static bool Armed(const OpenTreeNode* pNode) { return pNode->Armed(); }
    static bool Visited(const OpenTreeNode* pNode) { return pNode->bVisited; }
    static bool Flow(const OpenTreeNode* pNode) { return pNode->bFlow; }
    static bool VisitedFlow(const OpenTreeNode* pNode) { return pNode->bVisited && pNode->bFlow; }

    static bool True(const OpenTreeNode* pNode) { return true; }

public:
    OpenTree() {};
    ~OpenTree() {};

    void Process(const NodeOrder& _Ordering);

    void SerializeDotGraph(std::ostream& _Out) const;
    void DumpDotToFile(const std::string& _sPath) const;

private:
    OpenTreeNode* GetNode(BasicBlock* _pBB) const;

    void Prepare(NodeOrder& _Ordering);

    void AddNode(OpenTreeNode* _pNode);

    void Reroute(OpenSubTreeUnion& _Subtree);

    // return lowest ancestor of BB
    OpenTreeNode* InterleavePathsToBB(BasicBlock* _pBB);

    OpenTreeNode* CommonAncestor(BasicBlock* _pBB) const;

    template <class Container, class Filter, class Accessor> // Accessor extracts the OT node from the Container element, Filter processes the OT node and returns true or false
    std::vector<OpenTreeNode*> FilterNodes(const Container& _Container, const Filter& _Filter, const Accessor& _Accessor) const;

    // accessors for FilterNodes()
    OpenTreeNode* operator()(BasicBlock* _pBB) const { return GetNode(_pBB); }
    OpenTreeNode* operator()(OpenTreeNode* _pNode) const { return _pNode; }
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

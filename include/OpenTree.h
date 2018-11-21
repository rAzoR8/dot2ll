#pragma once

#include "NodeOrdering.h"

#include <unordered_map>
#include <unordered_set>

// forward delc
class OpenTree;
class Function;

struct OpenTreeNode
{
    OpenTreeNode(OpenTree* _pOT, BasicBlock* _pBB = nullptr);

    // is non-uniform (divergent) and one of the outgoing edges has already been closed
    bool Armed() const { return (pBB->GetTerminator() == nullptr ? pBB->GetDivergenceQualifier() : pBB->IsDivergent()) && uClosedOutgoing > 0u; }
    bool AncestorOf(const OpenTreeNode* _pSuccessor) const;
    OpenTreeNode* GetRoot();

    // called on predecesssor to close Pred->Succ, returns true if the OT changed
    void Close(OpenTreeNode* _Successor);

    std::string sName;

    std::vector<OpenTreeNode*> Children;

    uint32_t uClosedOutgoing = 0u;
    OpenTree* pOT = nullptr;
    OpenTreeNode* pParent = nullptr;
    BasicBlock* pBB = nullptr;
    bool bVisited = false; // has been added to the OT
    bool bFlow = false; // is a flowblock
    //bool bVirtual = false;

    struct Flow
    {
        OpenTreeNode* pTarget = nullptr;
        Instruction* pCondition = nullptr; // condition under which to branch to pTarget
    };

    // open edges
    std::vector<OpenTreeNode*> Incoming;
    std::vector<Flow> Outgoing;
    std::vector<Flow> FinalOutgoing; // only for closed outgoing flow

    static void LogTree(OpenTreeNode* _pNode = nullptr, std::string _sTabs = "");
};

struct FlowSuccessors
{
    std::vector<OpenTreeNode*> Vec; // successors of the flow node

    // from node OpenTreeNode* under condition  Instruction*
    using From = std::unordered_map<OpenTreeNode*, Instruction*>;
    
    // Target -> from (under condition)
    std::unordered_map<OpenTreeNode*, From> Conditions;

    void Add(OpenTreeNode* _pSource, OpenTreeNode* _pTarget, Instruction* _pCondition);
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
    const bool IsReconverging() const;

private:
    // order of nodes does not depend on memory address
    std::unordered_set<OpenTreeNode*, OTNHash> m_Nodes;
    std::unordered_set<OpenTreeNode*> m_Roots;
};

class OpenTree
{
    friend struct OpenTreeNode;
    static bool Armed(const OpenTreeNode* pNode) { return pNode->Armed(); }
    static bool Visited(const OpenTreeNode* pNode) { return pNode->bVisited; }
    static bool Unvisited(const OpenTreeNode* pNode) { return pNode->bVisited == false; }

    //static bool Flow(const OpenTreeNode* pNode) { return pNode->bFlow; }
    static bool VisitedFlow(const OpenTreeNode* pNode) { return pNode->bVisited && pNode->bFlow; }

    static bool True(const OpenTreeNode* pNode) { return true; }

public:
    OpenTree(const bool _bRemoveClosed = true, const std::string& _sDebugOutPath = "testoutput/") : 
        m_sDebugOutputPath(_sDebugOutPath), m_bRemoveClosed(_bRemoveClosed) {};
    ~OpenTree() {};

    // returns true if flow was rerouted or virtual nodes were inserted
    bool Process(const NodeOrder& _Ordering, const bool _bPrepareIfReconv, const bool _bPutVirtualFront = false);

    void SerializeOTDotGraph(std::ostream& _Out) const;
    void DumpOTDotToFile(const std::string& _sPath) const;

    void DumpCFGToFile(const std::string& _sPath);

private:
    OpenTreeNode* GetNode(BasicBlock* _pBB) const;

    bool Initialize(NodeOrder& _Ordering, const bool _bPrepareIfReconv, const bool _bPutVirtualFront);

    void AddNode(OpenTreeNode* _pNode);

    void Reroute(OpenSubTreeUnion& _Subtree);

    // return lowest ancestor of BB
    OpenTreeNode* InterleavePathsTo(OpenTreeNode* _pNode);

    OpenTreeNode* CommonAncestor(OpenTreeNode* _pNode) const;

    template <class OutputContainer = std::vector<OpenTreeNode*>, class Container, class Filter, class Accessor> // Accessor extracts the OT node from the Container element, Filter processes the OT node and returns true or false
    OutputContainer FilterNodes(const Container& _Container, const Filter& _Filter, const Accessor& _Accessor) const;

    void GetOutgoingFlow(std::vector<OpenTreeNode::Flow>& _OutFlow, OpenTreeNode* _pSource) const;

    // accessors for FilterNodes()
    OpenTreeNode* operator()(BasicBlock* _pBB) const { return GetNode(_pBB); }
    OpenTreeNode* operator()(OpenTreeNode* _pNode) const { return _pNode; }
    OpenTreeNode* operator()(const OpenTreeNode::Flow& _Flow) const { return _Flow.pTarget; }

private:
    uint32_t m_uNumFlowBlocks = 0u;
    std::vector<OpenTreeNode> m_Nodes;
    OpenTreeNode* m_pRoot = nullptr; // virtual root
    std::unordered_map<BasicBlock*, OpenTreeNode*> m_BBToNode;
    const bool m_bRemoveClosed;
    std::string m_sDebugOutputPath;
    Function* m_pFunction = nullptr;
};

template<class OutputContainer, class Container, class Filter, class Accessor>
inline OutputContainer OpenTree::FilterNodes(const Container& _Container, const Filter& _Filter, const Accessor& _Accessor) const
{
    OutputContainer Nodes;

    for (const auto& Elem : _Container)
    {
        if (OpenTreeNode* pNode = _Accessor(Elem); pNode != nullptr && _Filter(pNode))
        {
            Nodes.push_back(pNode);
        }
    }

    return Nodes;
}

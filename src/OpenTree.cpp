#include "OpenTree.h"
#include "ControlFlowGraph.h"

void OpenTree::Process(const NodeOrder& _Ordering)
{
    NodeOrder Ordering(_Ordering);

    Prepare(Ordering);

    std::vector<OpenTreeNode*> OutFlowNodes;

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering) // processNode(BBNode &Node)
    {
        // LLVM code ignores virtual nodes for rerouting, why?

        // Let P be the set of armed predecessors of B (non-uniform node with 1 open edge)
        std::vector<OpenTreeNode*> P = FilterNodes(B->GetPredecessors(), Armed);

        // If P is non-empty
        if (P.empty() == false)
        {
            // Let S be the set of subtrees rooted at nodes in P
            OpenSubTreeUnion S(P);

            // If S contains open outgoing edges that do not lead to B, reroute S Through a newly created basic block. FLOW
            if (S.HasOutgoingNotLeadingTo(B))
            {
                OutFlowNodes.push_back(Reroute(S)); // flow block is added to OT before B... problematic?
            }
        }

        // why add the node after armed predecessors have been defused?
        // => this makes sure B will be the PostDom.
        AddNode(B);

        // Let N be the set of visited successors of B, i.e. the targets of outgoing backward edges of N.
        std::vector<OpenTreeNode*> N = FilterNodes(B->GetSuccesors(), Visited);

        // If N is non-empty
        if (N.empty() == false)
        {
            // Let S be the set of subtrees routed at nodes in N.
            OpenSubTreeUnion S(N);

            // If S has multiple roots or open outgoing edges to multiple basic blocks, reroute S through a newly created basic block. FLOW
            if (S.HasMultiRootsOrOutgoing())
            {
                OutFlowNodes.push_back(Reroute(S));
            }
        }
    }

    // TODO: fix flow nodes OutFlowNodes
}

void OpenTree::Prepare(NodeOrder& _Ordering)
{
    NodeOrdering::PrepareOrdering(_Ordering);

    // reserve enough space for root & flow blocks
    m_Nodes.reserve(_Ordering.size() * 2u);
    m_pRoot = &m_Nodes.emplace_back();
    m_pRoot->sName = "ROOT";

    for (BasicBlock* B : _Ordering)
    {
        m_BBToNode[B] = &m_Nodes.emplace_back(B);
    }
}

void OpenTree::AddNode(BasicBlock* _pBB)
{
    OpenTreeNode* pNode = m_BBToNode[_pBB];
    const BasicBlock::Vec& Preds = _pBB->GetPredecessors();

    //if (Preds.size() == 0u)
    //{
    //    // if no predecessor of B is in OT, add B as a child of the root
    //    pNode->pParent = m_pRoot;        
    //}
    //else if (Preds.size() == 1u)
    //{
    //    // If a predecessor of B is already in OT, find the lowest predecessor(s).
    //    // If it is unique, add B as a child

    //    pNode->pParent = m_BBToNode[Preds[0]];
    //}
    //else
    //{
    //    pNode->pParent = InterleavePathsToBB(_pBB);
    //}

    // This should handle all cases:
    // if no common ancestor is found, root will be returned
    // otherwise the last leave of the merged path is returned

    pNode->pParent = InterleavePathsToBB(_pBB);
    pNode->pParent->Children.push_back(pNode);

    // close edge from Pred to BB
    // is this the right point to close the edge?
    // this changes the visited preds, so after interleaving makes sense
    for (BasicBlock* pPred : Preds)
    {
        OpenTreeNode* pPredNode = m_BBToNode[pPred];
        pPredNode->Close(_pBB);

        if (pPredNode->pFirstClosedSuccessor == nullptr)
        {
            pPredNode->pFirstClosedSuccessor = pNode;
        }
    }

    pNode->bVisited = true;
}

OpenTreeNode* OpenTree::Reroute(OpenSubTreeUnion& _Subtree)
{
    BasicBlock* pFlow = (*_Subtree.GetNodes().begin())->pBB->GetCFG()->NewNode("FLOW" + std::to_string(m_uNumFlowBlocks++));
    OpenTreeNode* pFlowNode = &m_Nodes.emplace_back(pFlow);
    pFlowNode->bFlow = true;
    m_BBToNode[pFlow] = pFlowNode;
    
    for (OpenTreeNode* pNode : _Subtree.GetNodes())
    {
        // pNode is (becomes) a predecessor of pFlow
        if (pNode->bFlow) // pNode is a flow node itself
        {
            // We need to split pNodes outgoing successors into 2 groups:
            // one that branches to the new pFlow block, and a original successor


        }
        else if(pNode->Outgoing.empty() == false)
        {
            // Regular nodes can only have up to 2 open outgoing nodes (because the ISA only has cond-branch, no switch)
            HASSERT(pNode->Outgoing.size() <= 2u, "Too many open outgoing edges");

            Instruction* pTerminator = pNode->pBB->GetTerminator();

            Instruction* pCondition = pTerminator->GetOperandInstr(0u); // nullptr if unconditional
            BasicBlock* pTrueSucc = nullptr;
            BasicBlock* pFalseSucc = nullptr;

            if (pTerminator->Is(kInstruction_Branch))
            {
                pTrueSucc = pTerminator->GetOperandBB(0u);
                HASSERT(pNode->Outgoing.size() == 1u && pNode->Outgoing[0] == pTrueSucc, "Open outgoin does not match branch successor");
            }
            else if (pTerminator->Is(kInstruction_BranchCond))
            {
                HASSERT(Contains(pNode->Outgoing, pTrueSucc) || Contains(pNode->Outgoing, pFalseSucc), "Outgoing BB does not match branch successor");

                pTrueSucc = pTerminator->GetOperandBB(1u);
                pFalseSucc = pTerminator->GetOperandBB(2u);

                // What is the case where the branch is conditional, but one open outgoing edge has been closed?
                // Armed only if the branch is also non-uniform

                if (pNode->Outgoing.size() == 1u)
                {
                    HLOG("Outgoing %s", WCSTR(pNode->Outgoing[0]->GetName().c_str()));
                }
            }

            // 1-to-1 association between OutgoingFlow and Incoming (open edges):
            OpenTreeNode::Flow& Flow = pFlowNode->OutgoingFlow.emplace_back();
            Flow.pSource = pNode;
            Flow.pCondition = pCondition; // if condition is nullptr -> unconditional branch (const true condition) -> FalseSucc = nullptr
            Flow.pTrueSucc = pTrueSucc;
            Flow.pFalseSucc = pFalseSucc;

            // reroute all outgoing edges to FlowBlock (convertes cond branch to branch)
            // LLVM code only does this if 2 outgoing edges were rerouted (i guess that the unconditional branch is simply reused/repurposed)
            pTerminator->Reset()->Branch(pFlow); // Branch from pNode to pFlow

            // TODO: check if outgoing (successor) is unvisited (LLVM code does so)

            // TODO: check if the edge Flow -> out already exisits

            // TODO: maybe put pNode into OpenTreeNode::Flow struct instead of Incoming open edges?
            //pFlowNode->Incoming.push_back(pNode->pBB);
            pNode->Outgoing = { pFlow }; // is this really an 'open' edge? 
        }
    }

    // add node to OT
    AddNode(pFlow);

    //if (pFlow->GetTerminator() == nullptr)
    //{
    //    // TODO: add conditon instr (can be nopped later)
    //    pFlow->AddInstruction()->Branch(pTarget);
    //}

    return pFlowNode;
}

// interleaves all node paths up until _pBB, returns last leave node (new ancestor)
OpenTreeNode* OpenTree::InterleavePathsToBB(BasicBlock* _pBB)
{
    std::deque<OpenTreeNode*> Branches;
    std::vector<OpenTreeNode*> Leaves;

    OpenTreeNode* pPrev = CommonAncestor(_pBB);

    for (OpenTreeNode* pBranch : pPrev->Children)
    {
        Branches.push_back(pBranch);
    }

    // traverse branches and concatenate
    while (Branches.empty() == false)
    {
        OpenTreeNode* pBranch = Branches.front();
        Branches.pop_front();

        if (pBranch->Children.empty())
        {
            Leaves.push_back(pBranch);
            continue;
        }
        else
        {
            // TODO: check if the node is an ancestor
            for (OpenTreeNode* pChild : pBranch->Children)
            {
                Branches.push_back(pChild);
            }
        }

        pPrev->Children = { pBranch };
        pBranch->pParent = pPrev;
        pPrev = pBranch;
    }

    // why do we need to merge leaves which are not ancestors?
    // pPrev->Children = Leaves;

    for (OpenTreeNode* pLeave : Leaves)
    {
        if (pLeave->pBB == _pBB) // skip target node
            continue;

        pPrev->Children = { pLeave };
        pLeave->pParent = pPrev;
        pPrev = pLeave;
    }

    return pPrev;
}

// returns root if non is found
OpenTreeNode* OpenTree::CommonAncestor(BasicBlock* _pBB) const
{
    std::unordered_set<OpenTreeNode*> Ancestors;

    std::deque<OpenTreeNode*> Nodes;

    auto VisitedPreds = FilterNodes(_pBB->GetPredecessors(), Visited);

    // find shared ancestor in visited predecessors
    for (OpenTreeNode* pVA : VisitedPreds)
    {
        Nodes.push_back(pVA);
    }

    while (Nodes.empty() == false)
    {
        OpenTreeNode* pAncestor = Nodes.front();
        Nodes.pop_front();

        bool bIsCommanAncestor = true;

        // check if this ancestor is an ancestor of all predecessors
        for (OpenTreeNode* pPred : VisitedPreds)
        {
            bIsCommanAncestor &= pAncestor->AncestorOf(pPred);
        }

        if (bIsCommanAncestor)
        {
            return pAncestor;
        }

        // add the ancestors ancestor
        if (pAncestor->pParent != nullptr)
        {
            Nodes.push_back(pAncestor->pParent);
        }
    }

    // todo: select lowest common ancestor?

    return m_pRoot;
}

bool OpenTreeNode::AncestorOf(const OpenTreeNode* _pSuccessor) const
{
    const OpenTreeNode* pNode = _pSuccessor;
    while (pNode != nullptr)
    {
        if (this == pNode->pParent)
            return true;

        pNode = pNode->pParent;
    }

    return false;
}

void OpenTreeNode::Close(BasicBlock* _OpenEdge, const bool _bRemoveClosed)
{
    if (auto it = std::remove(Incoming.begin(), Incoming.end(), _OpenEdge); it != Incoming.end())
    {
        Incoming.erase(it);
    }

    if (auto it = std::remove(Outgoing.begin(), Outgoing.end(), _OpenEdge); it != Outgoing.end())
    {
        Outgoing.erase(it);
    }

    if (_bRemoveClosed && pParent != nullptr && Outgoing.empty() && Incoming.empty())
    {        
        // move this nodes children to the parent
        for (OpenTreeNode* pChild : Children)
        {
            pParent->Children.push_back(pChild);
            pParent->Outgoing.push_back(pChild->pBB); // is this correct?
            pChild->pParent = pParent;
        }

        // close outgoing edge to this node
        //pParent->Close(pBB, _bRemoveClosed);
    }
};

OpenSubTreeUnion::OpenSubTreeUnion(const std::vector<OpenTreeNode*>& _Roots)
{
    for (OpenTreeNode* pRoot : _Roots)
    {
        // skip duplicates (backwardeges from armed preds)
        if (m_Nodes.count(pRoot) != 0)
            continue;

        m_Roots.insert(pRoot);

        std::deque<OpenTreeNode*> Stack = { pRoot }; // traversal stack
        
        while (Stack.empty() == false)
        {
            OpenTreeNode* pNode = Stack.back();
            Stack.pop_back();

            m_Nodes.insert(pNode);

            for (OpenTreeNode* pChild : pNode->Children)
            {
                if (m_Nodes.count(pChild) == 0)
                {
                    Stack.push_back(pChild);
                }
                else // node is a child => cannot be a root
                {
                    m_Roots.erase(pChild);
                }
            }
        }
    }
}

const bool OpenSubTreeUnion::HasOutgoingNotLeadingTo(BasicBlock* _pBB) const
{
    for (OpenTreeNode* pNode : m_Nodes)
    {
        if (pNode->Outgoing.empty() == false)
        {
            // outgoing has no open edge to B
            if (std::find(pNode->Outgoing.begin(), pNode->Outgoing.end(), _pBB) != pNode->Outgoing.end())
            {
                return true;
            }
        }
    }

    return false;
}

const bool OpenSubTreeUnion::HasMultiRootsOrOutgoing() const
{
    if (m_Roots.size() > 1u)
        return true;

    BasicBlock* pFirstOut = nullptr;
    for (OpenTreeNode* pNode : m_Nodes)
    {
        if (pFirstOut == nullptr)
        {
            if (pNode->Outgoing.empty() == false)
            {
                pFirstOut = pNode->Outgoing[0];
            }
        }

        if (pFirstOut != nullptr)
        {
            for (BasicBlock* pOut : pNode->Outgoing)
            {
                if (pFirstOut != pOut)
                    return true;
            }
        }
    }

    return false;
}
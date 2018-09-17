#include "OpenTree.h"
#include "ControlFlowGraph.h"

void OpenTree::Process(const NodeOrder& _Ordering)
{
    NodeOrder Ordering(_Ordering);

    Prepare(Ordering);

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering) // processNode(BBNode &Node)
    {
        // LLVM code ignores virtual nodes for rerouting, why?

        // Let P be the set of armed predecessors of B
        std::vector<OpenTreeNode*> P = FilterNodes(B->GetPredecessors(), Armed);

        // If P is non-empty
        if (P.empty() == false)
        {
            // Let S be the set of subtrees rooted at nodes in P
            OpenSubTreeUnion S(P);

            // If S contains open outgoing edges that do not lead to B, reroute S Through a newly created basic block. FLOW
            if (S.HasOutgoingNotLeadingTo(B))
            {
                Reroute(S);
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
                Reroute(S);
            }
        }
    }
}

void OpenTree::Prepare(NodeOrder& _Ordering)
{
    NodeOrdering::PrepareOrdering(_Ordering);

    // reserve enough space for root & flow blocks
    m_Nodes.reserve(_Ordering.size() * 2u);
    m_pRoot = &m_Nodes.emplace_back();

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

    // close edge from BB to Pred
    // is this the right point to close the edge?
    // this changes the visited preds, so after interleaving makes sense
    for (BasicBlock* pPred : Preds)
    {
        m_BBToNode[pPred]->Close(pPred);
    }

    pNode->bVisited = true;
}

void OpenTree::Reroute(const OpenSubTreeUnion& _Subtree)
{
    BasicBlock* pFlow = (*_Subtree.GetNodes().begin())->pBB->GetCFG()->AddNode("FLOW" + std::to_string(m_uNumFlowBlocks++));
    OpenTreeNode* pFlowNode = &m_Nodes.emplace_back(pFlow);
    m_BBToNode[pFlow] = pFlowNode;
    
    for (OpenTreeNode* pNode : _Subtree.GetNodes())
    {
        if (pNode->Outgoing.empty())
            continue;

        Instruction* pTerminator = pNode->pBB->GetTerminator();        

        if (pNode->Outgoing.size() == 1u)
        {
            pTerminator->Reset()->Branch(pFlow);
            
            if (pFlow->GetTerminator() == nullptr)
            {
                pFlow->AddInstruction()->Branch(pNode->Outgoing[0]);
            }

            pFlowNode->Incoming.push_back(pNode->Outgoing[0]);
        }
        else if (pNode->Outgoing.size() == 2u)
        {
            Instruction* pCondition = pTerminator->GetOperandInstr(0u);

            pTerminator->Reset()->Branch(pFlow);

            if (pFlow->GetTerminator() == nullptr)
            {
                pFlow->AddInstruction()->BranchCond(pCondition, pNode->Outgoing[0], pNode->Outgoing[1]);
            }

            pFlowNode->Incoming.push_back(pNode->Outgoing[0]);
            pFlowNode->Incoming.push_back(pNode->Outgoing[1]);
        }
        else
        {
            // TODO: 
        }

        pNode->Outgoing = { pFlow };      
    }

    AddNode(pFlow);
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

    const auto IsAncestorOf = [](OpenTreeNode* pAncestor, OpenTreeNode* pNode) -> bool
    {
        while (pNode != nullptr)
        {
            if (pAncestor == pNode->pParent)
                return true;

            pNode = pNode->pParent;
        }

        return false;
    };

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
            bIsCommanAncestor &= IsAncestorOf(pAncestor, pPred);
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

OpenSubTreeUnion::OpenSubTreeUnion(const std::vector<OpenTreeNode*> _Roots)
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

    OpenTreeNode* pOpenOutgoing = nullptr;
    for (OpenTreeNode* pNode : m_Nodes)
    {
        if (pOpenOutgoing == nullptr)
        {
            pOpenOutgoing = pNode;
        }
        else if (pOpenOutgoing != pNode)
        {
            return true;
        }
    }

    return false;
}

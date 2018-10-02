#include "OpenTree.h"
#include "ControlFlowGraph.h"

void OpenTree::Process(const NodeOrder& _Ordering)
{
    NodeOrder Ordering(_Ordering);

    Prepare(Ordering);

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering) // processNode(BBNode &Node)
    {
        OpenTreeNode* pNode = GetNode(B);
        // LLVM code ignores virtual nodes for rerouting, why?

        // Let P be the set of armed predecessors of B (non-uniform node with 1 open edge)
        // TODO: Predecessors are actually the FlowIncoming
        std::vector<OpenTreeNode*> P = FilterNodes(pNode->Incoming, Armed, *this);

        // If P is non-empty
        if (P.empty() == false)
        {
            // Let S be the set of subtrees rooted at nodes in P
            OpenSubTreeUnion S(P);

            // If S contains open outgoing edges that do not lead to B, reroute S Through a newly created basic block. FLOW
            if (S.HasOutgoingNotLeadingTo(B))
            {
                Reroute(S); // flow block is added to OT before B... problematic?
            }
        }

        // why add the node after armed predecessors have been defused?
        // => this makes sure B will be the PostDom.
        AddNode(B);

        // Let N be the set of visited successors of B, i.e. the targets of outgoing backward edges of N.
        // TODO: successors are actually the FlowOutgoing of B (open)
        std::vector<OpenTreeNode*> N = FilterNodes(pNode->Outgoing, Visited, *this);

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

OpenTreeNode* OpenTree::GetNode(BasicBlock* _pBB) const
{
    if (auto it = m_BBToNode.find(_pBB); it != m_BBToNode.end())
    {
        return it->second;
    }

    return nullptr;
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

    // TODO: in LLVM the predecessors are actually the open incoming edges from Flow nodes.
    const auto& Preds = pNode->Incoming;

    // TODO: LLVM code checks for VISITED preds! node can only be attached to a visited ancestor!

    if (Preds.size() == 0u)
    {
        // if no predecessor of B is in OT, add B as a child of the root
        pNode->pParent = m_pRoot;        
    }
    else if (Preds.size() == 1u)
    {
        // If a predecessor of B is already in OT, find the lowest predecessor(s).
        // If it is unique, add B as a child

        pNode->pParent = m_BBToNode[Preds[0]];
    }
    else
    {
        pNode->pParent = InterleavePathsToBB(_pBB);
    }

    // This should handle all cases:
    //pNode->pParent = InterleavePathsToBB(_pBB);

    // if no common ancestor is found, root will be returned
    // otherwise the last leave of the merged path is returned

    pNode->pParent->Children.push_back(pNode);

    // close edge from Pred to BB
    // is this the right point to close the edge?
    // this changes the visited preds, so after interleaving makes sense
    for (BasicBlock* pPred : Preds)
    {
        OpenTreeNode* pPredNode = m_BBToNode[pPred];

        if (pPredNode->pFirstClosedSuccessor == nullptr)
        {
            pPredNode->pFirstClosedSuccessor = pNode;
        }

        pPredNode->Close(pNode, true);
    }

    // TODO: close pNode if it has no open edges
    pNode->Close(nullptr, true);

    pNode->bVisited = true;
}

void OpenTree::Reroute(OpenSubTreeUnion& _Subtree)
{
    BasicBlock* pFlow = (*_Subtree.GetNodes().begin())->pBB->GetCFG()->NewNode("FLOW" + std::to_string(m_uNumFlowBlocks++));
    OpenTreeNode* pFlowNode = &m_Nodes.emplace_back(pFlow);
    pFlowNode->bFlow = true;
    m_BBToNode[pFlow] = pFlowNode;
    
    // accumulate all outgoing edges in the new flow node
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

            // ADD the outgoing flow from pBB to pFlow
            // TODO: check if outgoing (successor) is unvisited (LLVM code does so)
            OpenTreeNode::GetOutgoingFlowFromBB(pFlowNode->Outgoing, pNode->pBB);

            // reroute all outgoing edges to FlowBlock (convertes cond branch to branch)
            // LLVM code only does this if 2 outgoing edges were rerouted (i guess that the unconditional branch is simply reused/repurposed)
            pNode->pBB->GetTerminator()->Reset()->Branch(pFlow); // Branch from pNode to pFlow

            // this predecessor (pNode) is now an incomping edge to the flow node
            pFlowNode->Incoming.push_back(pNode->pBB);

            // SET outgoing flow pBB -> pFlow (uncond branch)
            pNode->Outgoing.clear();
            OpenTreeNode::GetOutgoingFlowFromBB(pNode->Outgoing, pNode->pBB);
        }
    }

    std::unordered_set<BasicBlock*> SuccTargets;
    for (OpenTreeNode::Flow& Succ : pFlowNode->Outgoing)
    {
        if (SuccTargets.count(Succ.pTarget) != 0u)
        {
            continue;
        }

        SuccTargets.insert(Succ.pTarget);

        // go over the unique successors of the flow block

        // Flow block is the incoming edge to the flow blocks outgoing BB
        m_BBToNode[Succ.pSource]->Incoming.push_back(pFlow);

        // LLVM code creates a PHI node for each of the Successors/Targets and adds it to the Flow BB
        // this phi node is the condition from all the Predecessors of the Target
    }

    // add node to OT
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

    std::deque<OpenTreeNode*> Nodes;

    auto VisitedPreds = FilterNodes(GetNode(_pBB)->Incoming /*_pBB->GetPredecessors()*/, Visited, *this);

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

OpenTreeNode::OpenTreeNode(BasicBlock* _pBB) :
    pBB(_pBB)
{
    if (_pBB != nullptr)
    {
        sName = _pBB->GetName();
        Incoming = _pBB->GetPredecessors();
        GetOutgoingFlowFromBB(Outgoing, _pBB);
    }
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

// close connection from predecessor (this) to the successor
void OpenTreeNode::Close(OpenTreeNode* _pSuccessor, const bool _bRemoveClosed)
{
    // LLVM code keeps track of all open in/out edges AND flow out edges seperately
    // here out flow edges are part of the Outgoing vector
    if (bFlow /*&& Outgoing.size() <= 2u && Incoming.empty()*/)
    {
        HASSERT(Outgoing.size() <= 2u, "Too many open outgoing flow edges");

        // create branch for out flow
        if (Outgoing.size() == 2u)
        {
            Instruction* pCondition = Outgoing[0].pCondition;
            HASSERT(pCondition != nullptr, "Invalid condtion (unconditional open edge)");
            if (Outgoing[0].bNot)
            {
                pCondition = pBB->AddInstruction()->Not(pCondition);
            }

            pBB->AddInstruction()->BranchCond(pCondition, Outgoing[0].pTarget, Outgoing[1].pTarget);
        }
        else if (Outgoing.size() == 1u)
        {
            pBB->AddInstruction()->Branch(Outgoing[0].pTarget);
        }

        // TODO: LLVM goes over Outgoing targets and removes the successor? from their incoming edges

        Outgoing.clear();
    }

    if (_pSuccessor != nullptr)
    {
        // this is the predecessor node, remove the successor
        if (auto it = std::remove_if(Outgoing.begin(), Outgoing.end(), [&](const Flow& _Flow) -> bool {return _Flow.pTarget == _pSuccessor->pBB; }); it != Outgoing.end())
        {
            Outgoing.erase(it);
        }

        // remove the predecessor from the successors incoming 
        if (auto it = std::remove(_pSuccessor->Incoming.begin(), _pSuccessor->Incoming.end(), pBB); it != _pSuccessor->Incoming.end())
        {
            _pSuccessor->Incoming.erase(it);
        }
    }

    // remove the node from the OT if all edges are closed
    if (_bRemoveClosed && Outgoing.empty() && Incoming.empty())
    {
        // move this nodes children to the parent
        for (OpenTreeNode* pChild : Children)
        {
            if (pParent != nullptr)
            {
                pParent->Children.push_back(pChild);
            }

            //pParent->Outgoing.push_back(pChild->pBB); // is this correct?
            pChild->pParent = pParent;
        }

        // remove the successor (child) from the parent
        if (pParent != nullptr && _pSuccessor != nullptr)
        {
            if (auto it = std::remove(pParent->Children.begin(), pParent->Children.end(), _pSuccessor); it != pParent->Children.end())
            {
                pParent->Children.erase(it);
            }
        }

        pParent = nullptr;
        Children.clear();
    }
}

void OpenTreeNode::GetOutgoingFlowFromBB(std::vector<Flow>& _OutFlow, BasicBlock* _pSource)
{
    Instruction* pTerminator = _pSource->GetTerminator();

    if (pTerminator == nullptr || _pSource->GetSuccesors().empty())
    {
        return;    
    }

    // TODO: check if the edge Flow -> out already exisits ?

    if (pTerminator->Is(kInstruction_Branch))
    {
        Flow& TrueFlow = _OutFlow.emplace_back();
        TrueFlow.pSource = _pSource;
        TrueFlow.pCondition = nullptr;
        TrueFlow.pTarget = pTerminator->GetOperandBB(1u);
    }
    else if (pTerminator->Is(kInstruction_BranchCond))
    {
        Flow& TrueFlow = _OutFlow.emplace_back();
        TrueFlow.pSource = _pSource;
        TrueFlow.pCondition = pTerminator->GetOperandInstr(0u);
        TrueFlow.pTarget = pTerminator->GetOperandBB(1u);

        Flow& FalseFlow = _OutFlow.emplace_back();
        FalseFlow.pSource = _pSource;
        FalseFlow.pCondition = TrueFlow.pCondition; // same condition instr
        FalseFlow.pTarget = pTerminator->GetOperandBB(2u); // false branch target
        FalseFlow.bNot = true; // negate condition

        // What is the case where the branch is conditional, but one open outgoing edge has been closed?
        // Armed only if the branch is also non-uniform

        //if (pNode->Outgoing.size() == 1u)
        //{
        //    HLOG("Outgoing %s", WCSTR(pNode->Outgoing[0]->GetName().c_str()));
        //}
    }
}

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
        for (OpenTreeNode::Flow& Out : pNode->Outgoing)
        {
            if (Out.pTarget == _pBB)
            {
                return false; // there is outgoing flow leading to B
            }
        }
    }

    return true;
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
                pFirstOut = pNode->Outgoing[0].pTarget;
            }
        }

        if (pFirstOut != nullptr)
        {
            for (const OpenTreeNode::Flow& Out : pNode->Outgoing)
            {
                if (pFirstOut != Out.pTarget)
                    return true;
            }
        }
    }

    return false;
}

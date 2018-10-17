#include "OpenTree.h"
#include "ControlFlowGraph.h"
#include "Function.h"

void OpenTree::Process(const NodeOrder& _Ordering)
{
    NodeOrder Ordering(_Ordering);

    Prepare(Ordering);

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering) // processNode(BBNode &Node)
    {
        //DumpDotToFile(B->GetName() + "_before.dot");

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
                Reroute(S);
            }
        }

        // why add the node after armed predecessors have been defused?
        // => this makes sure B will be the PostDom.
        AddNode(pNode);

        //DumpDotToFile(B->GetName() + "_after.dot");

        // Let N be the set of visited successors of B, i.e. the targets of outgoing backward edges of N.
        std::vector<OpenTreeNode*> N = FilterNodes(pNode->Outgoing, Visited, *this);

        // If N is non-empty
        if (N.empty() == false)
        {
            // Let S be the set of subtrees routed at nodes in N.
            OpenSubTreeUnion S(N);

            // close B -> visited Succ (needs to be changed if the filter above changes)
            for (OpenTreeNode* pSucc : N)
            {
                pNode->Close(pSucc, true);
            }

            // If S has multiple roots or open outgoing edges to multiple basic blocks, reroute S through a newly created basic block. FLOW
            if (S.HasMultiRootsOrOutgoing())
            {
                Reroute(S);
            }
        }
    }
}

void OpenTree::SerializeDotGraph(std::ostream& _Out) const
{
    _Out << "graph OT {\n";

    std::deque<OpenTreeNode*> Nodes = { m_pRoot };

    while (Nodes.empty() == false)
    {
        OpenTreeNode* pNode = Nodes.front();
        Nodes.pop_front();

#ifdef OUTGOING
        if (pNode->Outgoing.empty())
        {
            _Out << pNode->sName << ';' << std::endl;
        }

        for (const auto& flow : pNode->Outgoing)
        {
            OpenTreeNode* pSucc = GetNode(flow.pTarget);
            _Out << pNode->sName << " -- " << flow.pTarget->GetName();
#else
        if (pNode->Children.empty())
        {
            _Out << pNode->sName << ';' << std::endl;
        }

        for(OpenTreeNode* pSucc : pNode->Children)
        {
            _Out << pNode->sName << " -- " << pSucc->sName;
#endif
            _Out << "[label=";
            if (pSucc->bVisited)
            {
                _Out << "Visited";
            }
            _Out << "];" << std::endl;
            Nodes.push_back(pSucc);
        }
    }

    _Out << "}" << std::endl;
}

void OpenTree::DumpDotToFile(const std::string& _sPath) const
{
    std::ofstream stream(_sPath);
    if (stream.is_open())
    {
        SerializeDotGraph(stream);
        stream.close();
    }
}

void OpenTree::LogTree(OpenTreeNode* _pNode, std::string _sTabs) const
{
    if (_pNode == nullptr && m_pRoot != nullptr)
    {
        LogTree(m_pRoot);
    }
    else
    {
        std::string sOut, sIn;
        for (const OpenTreeNode* pIn : _pNode->Incoming)
        {
            sIn += ' ' + pIn->sName;
        }
        for (const OpenTreeNode::Flow& out : _pNode->Outgoing)
        {
            sOut += ' ' + out.pTarget->GetName();
        }

        HLOG("%s%s [IN:%s OUT:%s]", WCSTR(_sTabs), WCSTR(_pNode->sName), WCSTR(sIn), WCSTR(sOut));
        for (OpenTreeNode* pChild : _pNode->Children)
        {
            LogTree(pChild, _sTabs + '\t');
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
    m_pRoot = &m_Nodes.emplace_back(this, nullptr);
    m_pRoot->sName = "ROOT";
    m_pRoot->bVisited = true;

    HLOG("Node Order:");
    for (BasicBlock* B : _Ordering)
    {
        HLOG("\t%s", WCSTR(B->GetName()));
        m_BBToNode[B] = &m_Nodes.emplace_back(this, B);
    }

    // there is no outgoing edge from the ROOT to its successors (or incoming edge from the ROOT)

    // initialize open incoming and outgoing edges
    for (auto&[pBB, pNode] : m_BBToNode)
    {
        pNode->Incoming = FilterNodes(pBB->GetPredecessors(), True, *this);
        GetOutgoingFlow(pNode->Outgoing, pNode);
    }
}

void OpenTree::AddNode(OpenTreeNode* _pNode)
{
    HLOG(">>> AddNode %s", WCSTR(_pNode->sName));
    // LLVM code checks for VISITED preds, node can only be attached to a visited ancestor!
    // in LLVM the predecessors are actually the open incoming edges from FLOW nodes only. (IS THIS CORRECT?)
    const auto& Preds = FilterNodes(_pNode->Incoming, Visited, *this); // VisitedFlow ?

    if (Preds.size() == 0u)
    {
        // if no predecessor of B is in OT, add B as a child of the root
        _pNode->pParent = m_pRoot;
    }
    else if (Preds.size() == 1u)
    {
        // If a predecessor of B is already in OT, find the lowest predecessor(s).
        // If it is unique, add B as a child

        _pNode->pParent = Preds[0];
    }
    else
    {
        _pNode->pParent = InterleavePathsToBB(_pNode->pBB);
    }

    // This should handle all cases:
    //pNode->pParent = InterleavePathsToBB(_pBB);

    //HLOG("Adding Node %s -> %s", WCSTR(_pNode->pParent->sName), WCSTR(_pNode->sName));
    _pNode->pParent->Children.push_back(_pNode);

    LogTree();

    // close edge from Pred to BB
    // is this the right point to close the edge? LLVM code closes edges after adding for Predecessors and then for Successors.
    // this changes the visited preds, so after interleaving makes sense
    for (OpenTreeNode* pPred : Preds) // TODO: m_pRoot case is not in the Preds (need to add?)
    {
        if (pPred->pFirstClosedSuccessor == nullptr)
        {
            pPred->pFirstClosedSuccessor = _pNode;
        }

        pPred->Close(_pNode, true);
    }

    if (Preds.empty() == false)
    {
        LogTree();
    }

    if (_pNode->Incoming.empty() && _pNode->Outgoing.empty())
    {
        _pNode->Close(nullptr, true);
    }

    // can not close the edges to visited successors here because set N depends on the open edges.
    // question is if this is actually correct.

    _pNode->bVisited = true;
}

void OpenTree::Reroute(OpenSubTreeUnion& _Subtree)
{
    BasicBlock* pFlow = (*_Subtree.GetNodes().begin())->pBB->GetCFG()->NewNode("FLOW" + std::to_string(m_uNumFlowBlocks++));
    pFlow->SetDivergent(true);
    OpenTreeNode* pFlowNode = &m_Nodes.emplace_back(this, pFlow);
    pFlowNode->bFlow = true;
    m_BBToNode[pFlow] = pFlowNode;
    
    // accumulate all outgoing edges in the new flow node
    for (OpenTreeNode* pNode : _Subtree.GetNodes())
    {
        if (pNode->Outgoing.empty())
            continue;

        // this predecessor (pNode) is now an incoming edge to the flow node
        pFlowNode->Incoming.push_back(pNode);

        // pNode is (becomes) a predecessor of pFlow
        if (pNode->bFlow) // pNode is a flow node itself
        {
            // We need to split pNodes outgoing successors into 2 groups:
            // one that branches to the new pFlow block, and a original successor

            // is it possible that pNode->pFirstClosedSuccessor is null?

            Instruction* pCondition = nullptr;
            Instruction* pRemainderCond = nullptr;
            for (auto it = pNode->Outgoing.begin(); it != pNode->Outgoing.end();)
            {
                OpenTreeNode::Flow& out = *it;

                if (out.pTarget == pNode->pFirstClosedSuccessor->pBB) 
                {
                    Instruction* pNot = pFlow->AddInstruction()->Not(out.pCondition);
                    if (out.bNot)
                    {
                        pRemainderCond = out.pCondition;
                        pCondition = pNot;
                    }
                    else
                    {
                        pRemainderCond = pNot;
                        pCondition = out.pCondition;
                    }
                }
                else
                {
                    out.pSource = pNode->pBB;
                    pFlowNode->Outgoing.push_back(out);
                }

                it = pNode->Outgoing.erase(it);
            }

            // instead of Close we could create the final branch instruction here aswell?

            if (pCondition != nullptr)
            {
                auto& first = pNode->Outgoing.emplace_back();
                first.pCondition = pCondition;
                first.pSource = pNode->pBB;
                first.pTarget = pNode->pFirstClosedSuccessor->pBB;
            }

            // always route through the new flow block
            if (pRemainderCond == nullptr) 
            {
                pRemainderCond = pFlow->GetCFG()->GetFunction()->Constant(true);
            }

            auto& remain = pNode->Outgoing.emplace_back();
            remain.pCondition = pRemainderCond;
            remain.pSource = pNode->pBB;
            remain.pTarget = pFlow;
        }
        else // handle outgoing edges for non Flow Nodes
        {
            // Regular nodes can only have up to 2 open outgoing nodes (because the ISA only has cond-branch, no switch)
            HASSERT(pNode->Outgoing.size() <= 2u, "Too many open outgoing edges");
            Instruction* pTerminator = pNode->pBB->GetTerminator();
            Instruction* pCond = nullptr; bool bNot = false;
            
            if (pTerminator->Is(kInstruction_Branch))
            {
                HASSERT(pNode->Outgoing.size() == 1u, "Invalid number of outgoind edges");
                pTerminator->Reset()->Branch(pFlow);
            }
            else if(pTerminator->Is(kInstruction_BranchCond))
            {
                pCond = pTerminator->GetOperandInstr(0u);
                OpenTreeNode* pTrueNode = GetNode(pTerminator->GetOperandBB(1u));
                OpenTreeNode* pFalseNode = GetNode(pTerminator->GetOperandBB(2u));

                if (pTrueNode->bVisited == false && pFalseNode->bVisited == false)
                {
                    // rerouted both outgoing to the flow node, can replace with unconditional branch instr
                    pTerminator->Reset()->Branch(pFlow);
                }
                else if (pTrueNode->bVisited == false)
                {
                    pTerminator->Reset()->BranchCond(pCond, pFlow, pFalseNode->pBB);
                }
                else if (pFalseNode->bVisited == false)
                {
                    bNot = true;
                    pTerminator->Reset()->BranchCond(pCond, pTrueNode->pBB, pFlow);
                }

            }

             // transfer open outgoing edges to flow node
             pFlowNode->Outgoing.insert(pFlowNode->Outgoing.begin(), pNode->Outgoing.begin(), pNode->Outgoing.end());

            //for (const OpenTreeNode::Flow& out : pNode->Outgoing)
            //{
            //    if (GetNode(out.pTarget)->bVisited == false)
            //    {
            //        pFlowNode->Outgoing.push_back(out);
            //    }
            //}

            // SET outgoing flow pBB -> pFlow
            pNode->Outgoing.clear();
            GetOutgoingFlow(pNode->Outgoing, pNode); // now checks for unvisited
        }
    }

    std::string sOuts, sIns;
    // Successor -> Predecessor (get the predecessor for a certain successor)
    std::unordered_map<BasicBlock*, std::vector<OpenTreeNode::Flow*>> SuccTargets;
    for (OpenTreeNode::Flow& Succ : pFlowNode->Outgoing)
    {
        sOuts += ' ' + Succ.pTarget->GetName();
        sIns += ' ' + Succ.pSource->GetName();
        SuccTargets[Succ.pTarget].push_back(&Succ);
    }

    HLOG("Reroute%s -> %s ->%s", WCSTR(sIns), WCSTR(pFlow->GetName()), WCSTR(sOuts));

    // go over the unique successors of the flow block
    for (const auto&[pSucc, Preds] : SuccTargets)
    {
        // TODO: need to add false for preds of the flow node
        OpenTreeNode* pSuccNode = GetNode(pSucc);

        std::vector<Instruction*> Values;
        std::vector<BasicBlock*> Origins;
        for (OpenTreeNode::Flow* pPred : Preds)
        {
            // remove the source form the incoming of successor (pFlow is the only successor)
            OpenTreeNode* pPredNode = GetNode(pPred->pSource);
            if (auto it = std::remove(pSuccNode->Incoming.begin(), pSuccNode->Incoming.end(), pPredNode); it != pSuccNode->Incoming.end())
            {
                pSuccNode->Incoming.erase(it);
            }

            // create the conditon for the phi node
            Instruction* pCond = pPred->pCondition;
            if (pCond == nullptr) // unconditional
            {
                pCond = pFlow->GetCFG()->GetFunction()->Constant(true);
            }
            else if(pPred->bNot)
            {
                pCond = pFlow->AddInstruction()->Not(pCond);
            }

            Values.push_back(pCond);
            Origins.push_back(pPred->pSource);
        }

        // LLVM code creates a PHI node for each of the Successors/Targets and adds it to the Flow BB
        // this phi node is the condition from all the Predecessors of the Target
        pFlow->AddInstruction()->Phi(Values, Origins);

        // Flow block is the incoming edge to the flow blocks outgoing BB
        pSuccNode->Incoming.push_back(pFlowNode); // should the flow node be the only incoming edge to the successor?
    }

    // add node to OT
    AddNode(pFlowNode);
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

OpenTreeNode::OpenTreeNode(OpenTree* _pOT, BasicBlock* _pBB) :
    pOT(_pOT), pBB(_pBB)
{
    if (_pBB != nullptr)
    {
        sName = _pBB->GetName();
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
    if (_pSuccessor != nullptr) 
    {
        HLOG("Closing edge %s -> %s", WCSTR(sName), WCSTR(_pSuccessor->sName));    
    }

    if (_pSuccessor != nullptr)
    {
        // this is the predecessor node, remove the successor
        if (auto it = std::find_if(Outgoing.begin(), Outgoing.end(), [&](const Flow& _Flow) -> bool {return _Flow.pTarget == _pSuccessor->pBB; }); it != Outgoing.end())
        {
            if (bFlow)
            {
                FinalOutgoing.push_back(*it);
            }

            ++uClosedOutgoing;
            Outgoing.erase(it);
        }

        // remove the predecessor from the successors incoming 
        if (auto it = std::remove(_pSuccessor->Incoming.begin(), _pSuccessor->Incoming.end(), this); it != _pSuccessor->Incoming.end())
        {
            _pSuccessor->Incoming.erase(it);
        }
    }

    // remove the node from the OT if all edges are closed
    if (_bRemoveClosed && Outgoing.empty() && Incoming.empty())
    {
        // LLVM code keeps track of all open in/out edges AND flow out edges seperately
        // here the final outgoing flow is moved to FinalOutgoing when the edge is closed
        if (bFlow)
        {
            HASSERT(FinalOutgoing.size() <= 2u, "Too many open outgoing flow edges");

            // create branch for out flow
            if (FinalOutgoing.size() == 2u)
            {
                Instruction* pCondition = FinalOutgoing[0].pCondition;
                HASSERT(pCondition != nullptr, "Invalid condtion (unconditional open edge)");
                if (FinalOutgoing[0].bNot)
                {
                    pCondition = pBB->AddInstruction()->Not(pCondition);
                }

                pBB->AddInstruction()->BranchCond(pCondition, FinalOutgoing[0].pTarget, FinalOutgoing[1].pTarget);
                HLOG("BranchCond %s -> %s %s", WCSTR(pBB->GetName()), WCSTR(FinalOutgoing[0].pTarget->GetName()), WCSTR(FinalOutgoing[1].pTarget->GetName()));
            }
            else if (FinalOutgoing.size() == 1u)
            {
                pBB->AddInstruction()->Branch(FinalOutgoing[0].pTarget);
                HLOG("Branch %s -> %s", WCSTR(pBB->GetName()), WCSTR(Outgoing[0].pTarget->GetName()));
            }

            // remove this pred node from the incoming edges of the targets
            for (const OpenTreeNode::Flow& out : FinalOutgoing)
            {
                OpenTreeNode* pTargetNode = pOT->GetNode(out.pTarget);
                if (auto it = std::remove(pTargetNode->Incoming.begin(), pTargetNode->Incoming.end(), this); it != pTargetNode->Incoming.end())
                {
                    pTargetNode->Incoming.erase(it);
                }
            }
        }

        HLOG("Closing node %s", WCSTR(sName));
        // move this nodes children to the parent
        for (OpenTreeNode* pChild : Children)
        {
            if (pParent != nullptr)
            {
                pParent->Children.push_back(pChild);
            }

            pChild->pParent = pParent;
        }

        // remove the child from the parent
        if (pParent != nullptr)
        {
            if (auto it = std::remove(pParent->Children.begin(), pParent->Children.end(), this); it != pParent->Children.end())
            {
                pParent->Children.erase(it);
            }

            HASSERTD(std::find(pParent->Children.begin(), pParent->Children.end(), this) == pParent->Children.end(), "Duplicate");
        }

        // this node is removed from the OT, it has no ancestor or successor
        pParent = nullptr;
        Children.clear();
    }
}

void OpenTree::GetOutgoingFlow(std::vector<OpenTreeNode::Flow>& _OutFlow, OpenTreeNode* _pSource) const
{
    Instruction* pTerminator = _pSource->pBB->GetTerminator();

    if (pTerminator == nullptr || _pSource->pBB->GetSuccesors().empty())
    {
        return;    
    }

    if (pTerminator->Is(kInstruction_Branch))
    {
        if (OpenTreeNode* pTarget = GetNode(pTerminator->GetOperandBB(0u)); pTarget->bVisited == false)
        {
            OpenTreeNode::Flow& TrueFlow = _OutFlow.emplace_back();
            TrueFlow.pSource = _pSource->pBB;
            TrueFlow.pCondition = nullptr;
            TrueFlow.pTarget = pTarget->pBB;
        }
    }
    else if (pTerminator->Is(kInstruction_BranchCond))
    {
        if (OpenTreeNode* pTarget = GetNode(pTerminator->GetOperandBB(1u)); pTarget->bVisited == false)
        {
            OpenTreeNode::Flow& TrueFlow = _OutFlow.emplace_back();
            TrueFlow.pSource = _pSource->pBB;
            TrueFlow.pCondition = pTerminator->GetOperandInstr(0u);
            TrueFlow.pTarget = pTarget->pBB;
        }

        if (OpenTreeNode* pTarget = GetNode(pTerminator->GetOperandBB(2u)); pTarget->bVisited == false)
        {
            OpenTreeNode::Flow& FalseFlow = _OutFlow.emplace_back();
            FalseFlow.pSource = _pSource->pBB;
            FalseFlow.pCondition = pTerminator->GetOperandInstr(0u); // same condition instr
            FalseFlow.pTarget = pTarget->pBB; // false branch target
            FalseFlow.bNot = true; // negate condition
        }
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
            if (Out.pTarget != _pBB) // LLVM code also checks if target is UNVISITED (should be the case here)
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
        // LLVM code checks for UNVISITED pNode->Visited() == false, why?
        // because visited also means the node has been rerouted eventually?

        if (pNode->bVisited)
        {
            continue;
        }

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

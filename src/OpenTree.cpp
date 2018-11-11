#include "OpenTree.h"
#include "ControlFlowGraph.h"
#include "Function.h"
#include "CheckReconvergence.h"
#include <deque>

void FlowSuccessors::Add(OpenTreeNode* _pSource, OpenTreeNode* _pTarget, Instruction* _pCondition)
{
    if (Conditions.count(_pTarget) == 0u)
    {
        Vec.emplace_back(_pTarget);
    }
    Conditions[_pTarget][_pSource] = _pCondition;
}

bool OpenTree::Process(const NodeOrder& _Ordering, const bool _bPrepareIfReconv, const bool _bPutVirtualFront)
{
    bool bRerouted = false;

    NodeOrder Ordering(_Ordering);

    Initialize(Ordering, _bPrepareIfReconv, _bPutVirtualFront);

    // root
    DumpDotToFile("0_step.dot");

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering) // processNode(BBNode &Node)
    {
        uint32_t uStep = 0u;
        OpenTreeNode* pNode = GetNode(B);

        HLOG(">>> Processing %s", WCSTR(pNode->sName));

        //if (B->IsVirtual() == false)
        {
            // Let P be the set of armed predecessors of B (non-uniform node with 1 open edge)
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
                    bRerouted = true;
                    DumpDotToFile(B->GetName() + "_step" + std::to_string(uStep++) + ".dot");
                }
            }
        }
        
        AddNode(pNode);

        DumpDotToFile(B->GetName() + "_step" + std::to_string(uStep++) + ".dot");

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
                pNode->Close(pSucc);                
            }

            // If S has multiple roots or open outgoing edges to multiple basic blocks, reroute S through a newly created basic block. FLOW
            if (S.HasMultiRootsOrOutgoing())
            {
                Reroute(S);
                bRerouted = true;
                DumpDotToFile(B->GetName() + "_step" + std::to_string(uStep++) + ".dot");
            }
        }
    }

    HASSERT(m_pRoot->Children.empty(), "Unresolved nodes");

    return bRerouted;
}

void OpenTree::SerializeDotGraph(std::ostream& _Out) const
{
    _Out << "digraph OT {\nnode[shape = box]\n";

    std::deque<OpenTreeNode*> Nodes = { m_pRoot };

    while (Nodes.empty() == false)
    {
        OpenTreeNode* pNode = Nodes.front();
        Nodes.pop_front();

        bool bPrinted = false;

        if (pNode->pBB != nullptr && pNode->Armed())
        {
            _Out << pNode->sName << "[color=red];" << std::endl;
            bPrinted = true;
        }

        for (const OpenTreeNode* pIn : pNode->Incoming)
        {
            _Out << pIn->sName << " -> " << pNode->sName << "[style=dashed];" << std::endl;
            bPrinted = true;
        }

        for (const auto& flow : pNode->Outgoing)
        {
            _Out << pNode->sName << " -> " << flow.pTarget->sName << "[style=dotted];" << std::endl;
            bPrinted = true;
        }

        for(OpenTreeNode* pSucc : pNode->Children)
        {
            bPrinted = true;
            _Out << pNode->sName << " -> " << pSucc->sName << ';' << std::endl;
            Nodes.push_back(pSucc);
        }

        if (bPrinted == false)
        {
            _Out << pNode->sName << ';' << std::endl;
        }
    }

    _Out << "}" << std::endl;
}

void OpenTree::DumpDotToFile(const std::string& _sPath) const
{
    std::ofstream stream(m_sDebugOutputPath + _sPath);
    if (stream.is_open())
    {
        SerializeDotGraph(stream);
        stream.close();
    }
}

void OpenTreeNode::LogTree(OpenTreeNode* _pNode, std::string _sTabs)
{
    if (_pNode != nullptr)
    {
        std::string sOut, sIn;
        for (const OpenTreeNode* pIn : _pNode->Incoming)
        {
            sIn += ' ' + pIn->sName;
        }
        for (const auto& out : _pNode->Outgoing)
        {
            sOut += ' ' + out.pTarget->sName;
        }

        HLOG("%s%s [IN:%s OUT:%s] %s", WCSTR(_sTabs), WCSTR(_pNode->sName), WCSTR(sIn), WCSTR(sOut), (_pNode->pBB != nullptr && _pNode->Armed()) ? L"armed" : L"");
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

void OpenTree::Initialize(NodeOrder& _Ordering, const bool _bPrepareIfReconv, const bool _bPutVirtualFront)
{
    // only execute if nodes in ordering are not reconverging already
    if (_bPrepareIfReconv || CheckReconvergence::IsReconverging(_Ordering) == false)
    {
        NodeOrdering::PrepareOrdering(_Ordering, _bPutVirtualFront);
    }

    // reserve enough space for root & flow blocks
    m_Nodes.reserve(_Ordering.size() * 2u);
    m_pRoot = &m_Nodes.emplace_back(this, nullptr);
    m_pRoot->sName = "ROOT";
    m_pRoot->bVisited = true;

    HLOG("Node Order [%d]:", _Ordering.size());
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
    HLOG("AddNode %s", WCSTR(_pNode->sName));
    // LLVM code checks for VISITED preds, node can only be attached to a visited ancestor!
    // in LLVM the predecessors are actually the open incoming edges from FLOW nodes only. (IS THIS CORRECT?)
    const auto& Preds = FilterNodes(_pNode->Incoming, Visited, *this);

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
        _pNode->pParent = InterleavePathsTo(_pNode);
    }

    // This should handle all cases:
    //pNode->pParent = InterleavePathsToBB(_pBB);

    HLOG("Attaching Node %s -> %s", WCSTR(_pNode->pParent->sName), WCSTR(_pNode->sName));
    _pNode->pParent->Children.push_back(_pNode);

    OpenTreeNode::LogTree(m_pRoot);

    // close edge from Pred to BB
    // is this the right point to close the edge? LLVM code closes edges after adding for Predecessors and then for Successors.
    // this changes the visited preds, so after interleaving makes sense
    for (OpenTreeNode* pPred : Preds) // TODO: m_pRoot case is not in the Preds (need to add?)
    {
        pPred->Close(_pNode);
    }

    // can not close the edges to visited successors here because set N depends on the open edges.
    // question is if this is actually correct.

    _pNode->bVisited = true;
}

void OpenTree::Reroute(OpenSubTreeUnion& _Subtree)
{
    BasicBlock* pFlow = (*_Subtree.GetNodes().begin())->pBB->GetCFG()->NewNode("FLOW" + std::to_string(m_uNumFlowBlocks++));
    OpenTreeNode* pFlowNode = &m_Nodes.emplace_back(this, pFlow);
    pFlowNode->bFlow = true;
    m_BBToNode[pFlow] = pFlowNode;
    FlowSuccessors S;

    Function& Func(*pFlow->GetCFG()->GetFunction());
    Instruction* pConstTrue = Func.Constant(true);
    Instruction* pConstFalse = Func.Constant(false);
        
    std::string sOuts, sIns;
    
    // accumulate all outgoing edges in the new flow node
    for (OpenTreeNode* pNode : _Subtree.GetNodes())
    {
        if (pNode->Outgoing.empty())
            continue;

        // this predecessor (pNode) is now an incoming edge to the flow node
        pFlowNode->Incoming.push_back(pNode);
        sIns += ' ' + pNode->sName;

        // pNode is (becomes) a predecessor of pFlow
        if (pNode->bFlow) // pNode is a flow node itself
        {
            // We need to split pNodes outgoing successors into 2 groups:
            // one that branches to the new pFlow block, and a original successor

            Instruction* pRemainderCond = nullptr;

            if (pNode->FinalOutgoing.empty() == false)
            {
                Instruction* pCond = pNode->FinalOutgoing.front().pCondition;
                pRemainderCond = pCond->GetBasicBlock()->InsertInstructionAfter(pCond)->Not(pCond);

                for (auto it = pNode->FinalOutgoing.begin()+1; it != pNode->FinalOutgoing.end();)
                {
                    S.Add(pNode, it->pTarget, it->pCondition);
                    it = pNode->FinalOutgoing.erase(it);
                }                
            }

            for (auto it = pNode->Outgoing.begin(); it != pNode->Outgoing.end();)
            {
                S.Add(pNode, it->pTarget, it->pCondition); 
                it = pNode->Outgoing.erase(it);
            }

            // always route through the new flow block
            auto& remain = pNode->Outgoing.emplace_back();
            remain.pCondition = pRemainderCond != nullptr ? pRemainderCond : pConstTrue;
            remain.pTarget = pFlowNode;
        }
        else // handle outgoing edges for non Flow Nodes
        {
            // Regular nodes can only have up to 2 open outgoing nodes (because the ISA only has cond-branch, no switch)
            HASSERT(pNode->Outgoing.size() <= 2u, "Too many open outgoing edges");
            Instruction* pTerminator = pNode->pBB->GetTerminator();
              
            if (pTerminator->Is(kInstruction_Branch))
            {
                HASSERT(pNode->Outgoing.size() == 1u, "Invalid number of outgoind edges");
                S.Add(pNode, GetNode(pTerminator->GetOperandBB(0u)), pConstTrue);
                pTerminator->Reset()->Branch(pFlow);
            }
            else if(pTerminator->Is(kInstruction_BranchCond))
            {
                Instruction* pCond = pTerminator->GetOperandInstr(0u);
                OpenTreeNode* pTrueNode = GetNode(pTerminator->GetOperandBB(1u));
                OpenTreeNode* pFalseNode = GetNode(pTerminator->GetOperandBB(2u));

                if (pTrueNode->bVisited == false && pFalseNode->bVisited)
                {
                    S.Add(pNode, pTrueNode, pConstTrue);
                    pTerminator->Reset()->BranchCond(pCond, pFlow, pFalseNode->pBB);
                }
                if (pFalseNode->bVisited == false && pTrueNode->bVisited)
                {
                    S.Add(pNode, pFalseNode, pConstTrue);
                    pTerminator->Reset()->BranchCond(pCond, pTrueNode->pBB, pFlow);
                }
                if (pTrueNode->bVisited == false && pFalseNode->bVisited == false)
                {
                    // rerouted both outgoing to the flow node, can replace with unconditional branch instr
                    S.Add(pNode, pTrueNode, pCond);
                    S.Add(pNode, pFalseNode, pTerminator->Reset()->Not(pCond));
                    pNode->pBB->AddInstruction()->Branch(pFlow);
                }
            }

            // SET outgoing flow pBB -> pFlow
            pNode->Outgoing.clear();
            GetOutgoingFlow(pNode->Outgoing, pNode); // now checks for unvisited
        }
    }

    // go over the unique successors of the flow block
    for (OpenTreeNode* pFlowSucc : S.Vec)
    {
        sOuts += ' ' + pFlowSucc->sName;

        const FlowSuccessors::From& Conditions = S.Conditions[pFlowSucc];

        std::vector<Instruction*> Values; std::vector<BasicBlock*> Origins;
        // create the conditons for the phi node
        for (OpenTreeNode* pFlowPred : pFlowNode->Incoming)
        {
            if (auto it = Conditions.find(pFlowPred); it != Conditions.end())
            {
                Values.push_back(it->second);
            }
            else
            {
                Values.push_back(pConstFalse);
            }

            Origins.push_back(pFlowPred->pBB);

            // remove the source form the incoming of successor (pFlow is the only successor)
            if (auto it = std::remove(pFlowSucc->Incoming.begin(), pFlowSucc->Incoming.end(), pFlowPred); it != pFlowSucc->Incoming.end())
            {
                pFlowSucc->Incoming.erase(it);
            }
        }

        // TODO: print PHI node


        // flow successors
        auto& out = pFlowNode->Outgoing.emplace_back();
        out.pTarget = pFlowSucc;

        // this phi node is the condition from all the Predecessors of the Target

        out.pCondition = pFlow->AddInstruction()->Phi(Values, Origins);

        // Flow block is the incoming edge to the flow blocks outgoing BB
        pFlowSucc->Incoming.push_back(pFlowNode);
    }

    HLOG("Reroute%s -> %s ->%s", WCSTR(sIns), WCSTR(pFlow->GetName()), WCSTR(sOuts));
    
    // add node to OT
    AddNode(pFlowNode);
}

// interleaves all node paths up until _pNode, returns last leave node (new ancestor)
OpenTreeNode* OpenTree::InterleavePathsTo(OpenTreeNode* _pNode)
{
    std::deque<OpenTreeNode*> Branches;
    std::vector<OpenTreeNode*> Leaves;

    OpenTreeNode* pPrev = CommonAncestor(_pNode);

    HLOG("%s is common ancestor of %s", WCSTR(pPrev->sName), WCSTR(_pNode->sName));

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

        //HLOG("Attaching %s to %s", WCSTR(pBranch->sName), WCSTR(pPrev->sName));
        pPrev->Children = { pBranch };
        pBranch->pParent = pPrev;
        pPrev = pBranch;
    }

    // why do we need to merge leaves which are not ancestors?
    // pPrev->Children = Leaves;

    for (OpenTreeNode* pLeave : Leaves)
    {
        if (pLeave == _pNode) // skip target node
            continue;

        //HLOG("Attaching %s to %s", WCSTR(pLeave->sName), WCSTR(pPrev->sName));
        pPrev->Children = { pLeave };
        pLeave->pParent = pPrev;
        pPrev = pLeave;
    }

    return pPrev;
}

// returns root if non is found
OpenTreeNode* OpenTree::CommonAncestor(OpenTreeNode* _pNode) const
{
    std::unordered_set<OpenTreeNode*> Ancestors;

    std::deque<OpenTreeNode*> Nodes;

    auto VisitedPreds = FilterNodes(_pNode->Incoming, Visited, *this);

    // find shared ancestor in visited predecessors
    for (OpenTreeNode* pVA : VisitedPreds)
    {
        Nodes.push_back(pVA);
    }

    while (Nodes.empty() == false)
    {
        OpenTreeNode* pAncestor = Nodes.front();
        Nodes.pop_front();

        //HLOG("Checking ancestor %s", WCSTR(pAncestor->sName));

        bool bIsCommanAncestor = true;

        // check if this ancestor is an ancestor of all predecessors
        for (OpenTreeNode* pPred : VisitedPreds)
        {
            bool bAncestor = false;
            if (pPred != pAncestor)
            {
                bAncestor = pAncestor->AncestorOf(pPred);
                HLOG("%s %s ancestor of %s", WCSTR(pAncestor->sName), bAncestor ? L"is" : L"is not", WCSTR(pPred->sName));
            }

            bIsCommanAncestor &= bAncestor;
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
    HWARNING("Did not find a commong ancestor, using root");

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

OpenTreeNode* OpenTreeNode::GetRoot()
{
    if (pParent == nullptr)
    {
        return this;
    }

    return pParent->GetRoot();
}

// close connection from predecessor (this) to the successor
void OpenTreeNode::Close(OpenTreeNode* _pSuccessor)
{
    if (_pSuccessor != nullptr) 
    {
        HLOG("Closing edge %s -> %s", WCSTR(sName), WCSTR(_pSuccessor->sName));    
    }

    if (_pSuccessor != nullptr)
    {
        // this is the predecessor node, remove the successor
        if (auto it = std::find_if(Outgoing.begin(), Outgoing.end(), [&](const Flow& _Flow) -> bool {return _Flow.pTarget == _pSuccessor; }); it != Outgoing.end())
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

    if (_pSuccessor != nullptr && _pSuccessor->Incoming.empty() && _pSuccessor->Outgoing.empty())
    {
        _pSuccessor->Close(nullptr);
    }

    // remove the node from the OT if all edges are closed
    if (pOT->m_bRemoveClosed && Outgoing.empty() && Incoming.empty())
    {
        OpenTreeNode* pRoot = GetRoot();
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
                pBB->AddInstruction()->BranchCond(pCondition, FinalOutgoing[0].pTarget->pBB, FinalOutgoing[1].pTarget->pBB);
                HLOG("BranchCond %s -> %s %s", WCSTR(pBB->GetName()), WCSTR(FinalOutgoing[0].pTarget->sName), WCSTR(FinalOutgoing[1].pTarget->sName));
            }
            else if (FinalOutgoing.size() == 1u)
            {
                pBB->AddInstruction()->Branch(FinalOutgoing[0].pTarget->pBB);
                HLOG("Branch %s -> %s", WCSTR(pBB->GetName()), WCSTR(FinalOutgoing[0].pTarget->sName));
            }
        }

        HLOG("Closing node %s", WCSTR(sName));
        // move this nodes children to the parent
        for (OpenTreeNode* pChild : Children)
        {
            if (pParent != nullptr)
            {
                HLOG("Moving child %s to %s", WCSTR(pChild->sName), WCSTR(pParent->sName));
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

        LogTree(pRoot);
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
            auto& TrueFlow = _OutFlow.emplace_back();
            TrueFlow.pCondition = _pSource->pBB->GetCFG()->GetFunction()->Constant(true);
            TrueFlow.pTarget = pTarget;
        }
    }
    else if (pTerminator->Is(kInstruction_BranchCond))
    {
        if (OpenTreeNode* pTarget = GetNode(pTerminator->GetOperandBB(1u)); pTarget->bVisited == false)
        {
            auto& TrueFlow = _OutFlow.emplace_back();
            TrueFlow.pCondition = pTerminator->GetOperandInstr(0u);
            TrueFlow.pTarget = pTarget;
        }

        if (OpenTreeNode* pTarget = GetNode(pTerminator->GetOperandBB(2u)); pTarget->bVisited == false)
        {
            auto& FalseFlow = _OutFlow.emplace_back();
            FalseFlow.pCondition = pTerminator->GetOperandInstr(0u); // same condition instr
            FalseFlow.pTarget = pTarget; // false branch target
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
        for (auto& Out : pNode->Outgoing)
        {
            if (Out.pTarget->pBB != _pBB) // LLVM code also checks if target is UNVISITED (should be the case here)
            {
                return true;
            }
        }
    }

    return false;
}

const bool OpenSubTreeUnion::HasMultiRootsOrOutgoing() const
{
    // TODO: check if roots have open outgoing edges, otherwise theres nothing to reroute
    if (m_Roots.size() > 1u)
        return true;

    OpenTreeNode* pFirstOut = nullptr;
    for (OpenTreeNode* pNode : m_Nodes)
    {
        for (const auto& Out : pNode->Outgoing)
        {
            if (Out.pTarget->bVisited == false)
            {
                if (pFirstOut == nullptr)
                {
                    pFirstOut = Out.pTarget;
                }
                else if (pFirstOut != Out.pTarget)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

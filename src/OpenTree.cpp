#include "OpenTree.h"
#include <unordered_set>

void OpenTree::Process(const NodeOrder& _Ordering)
{
    NodeOrder Ordering(_Ordering);

    Prepare(Ordering);

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering)
    {
        // Let P be the set of armed predecessors of B
        std::vector<OpenTreeNode*> P = FilterNodes(B->GetPredecessors(), Armed);

        // If P is non-empty
        if (P.empty() == false)
        {
            // Let S be the set of subtrees rooted at nodes in P

            // If S contains open outgoing edges that do not lead to B, reroute S Through a newly created basic block. FLOW
        }

        AddNode(B);

        // Let N be the set of visited successors of B, i.e. the targets of outgoing backward edges of N.
        std::vector<OpenTreeNode*> N = FilterNodes(B->GetSuccesors(), Visited);

        // If N is non-empty
        if (N.empty() == false)
        {
            // Let S be the set of subtrees routed at nodes in N.

            // If S has multiple roots or open outgoing edges to multiple basic blocks, reroute S through a newly created basic block. FLOW
        }
    }
}

void OpenTree::Prepare(NodeOrder& _Ordering)
{
    NodeOrdering::PrepareOrdering(_Ordering);

    m_Nodes.reserve(_Ordering.size() + 1u);
    m_pRoot = &m_Nodes.emplace_back();

    for (BasicBlock* B : _Ordering)
    {
        m_BBToNode[B] = &m_Nodes.emplace_back(B);
    }
}

void OpenTree::AddNode(BasicBlock* _pBB)
{
    OpenTreeNode* pNode = m_BBToNode[_pBB];
    const auto& Preds = _pBB->GetPredecessors();

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

    pNode->pParent->Children.push_back(pNode);
}

OpenTreeNode* OpenTree::InterleavePathsToBB(BasicBlock* _pBB)
{
    OpenTreeNode* pCommonAncestor = CommonAncestor(_pBB);

    // TODO: traverse OT to leaves and merge paths (separate leave nodes because they have to come last)

    OpenTreeNode* pLowestAncestor = m_pRoot;

    return pLowestAncestor;
}

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

    return nullptr;
}

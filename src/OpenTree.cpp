#include "OpenTree.h"

void OpenTree::Process(const NodeOrder& _Ordering)
{
    NodeOrder Ordering(_Ordering);

    Prepare(Ordering);

    // For each basic block B in the ordering
    for (BasicBlock* B : Ordering)
    {
        // Let P be the set of armed predecessors of B
        std::vector<OpenTreeNode*> P = GetArmedPredecessors(B);

        // If P is non-empty
        if (P.empty() == false)
        {
            // Let S be the set of subtrees rooted at nodes in P

            // If S contains open outgoing edges that do not lead to B, reroute S Through a newly created basic block. FLOW
        }

        AddNode(B);

        // Let N be the set of visited successors of B, i.e. the targets of outgoing backward edges of N.
        std::vector<OpenTreeNode*> N = GetVistedSuccessors(B);

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
        // TODO: interleave
    }

    pNode->pParent->Children.push_back(pNode);
}

std::vector<OpenTreeNode*> OpenTree::GetArmedPredecessors(BasicBlock* _pBB) const
{
    std::vector<OpenTreeNode*> ArmedPreds;

    for (BasicBlock* pPred : _pBB->GetPredecessors())
    {
        if (auto it = m_BBToNode.find(pPred); it != m_BBToNode.end())
        {
            if (it->second->Armed())
            {
                ArmedPreds.push_back(it->second);
            }
        }
    }

    return ArmedPreds;
}

std::vector<OpenTreeNode*> OpenTree::GetVistedSuccessors(BasicBlock* _pBB) const
{
    std::vector<OpenTreeNode*> VisitedSuccessors;

    for (BasicBlock* pSucc : _pBB->GetSuccesors())
    {
        if (auto it = m_BBToNode.find(pSucc); it != m_BBToNode.end())
        {
            if (it->second->Visited())
            {
                VisitedSuccessors.push_back(it->second);
            }
        }
    }

    return VisitedSuccessors;
}

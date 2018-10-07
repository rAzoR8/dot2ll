#include "NodeOrdering.h"
#include "CFGUtils.h"

NodeOrder NodeOrdering::ComputeDepthFirst(BasicBlock* _pRoot)
{
    NodeOrder Order;
    BasicBlock* pExit = nullptr;

    for (BasicBlock* pBB : CFGUtils::DepthFirst(_pRoot))
    {
        if (pBB->IsSink())
        {
            pExit = pBB;
            continue;
        }

        Order.push_back(pBB);
    }

    // make sure the exite node comes last in the ordering
    if (pExit != nullptr)
    {
        Order.push_back(pExit);
    }

    return Order;
}

NodeOrder NodeOrdering::ComputeBreadthFirst(BasicBlock* _pRoot)
{
    NodeOrder Order;

    std::unordered_set<BasicBlock*> traversed;
    std::list<BasicBlock*> frontier = {_pRoot};

    const auto AncestorsTraversed = [&traversed](BasicBlock* _pBB) -> bool
    {
        for (BasicBlock* pAncestor : _pBB->GetPredecessors())
        {
            if (pAncestor != _pBB) // ignore loops / backward edges to self
            {
                if (traversed.count(pAncestor) == 0) // not traversed yet
                    return false;
            }
        }

        return true;
    };

    while (frontier.empty() == false)
    {
        for (auto it = frontier.begin(); it != frontier.end();)
        {
            BasicBlock* pBB = *it;

            if (AncestorsTraversed(pBB))
            {
                traversed.insert(pBB);
                Order.push_back(pBB);
                it = frontier.erase(it);

                for (BasicBlock* pSuccessor : pBB->GetSuccesors())
                {
                    if (pSuccessor != pBB && traversed.count(pSuccessor) == 0 && Contains(frontier, pSuccessor) == false) // ignore backward eges / loops
                    {
                        frontier.push_front(pSuccessor);
                    }
                }
            }
            else
            {
                for (BasicBlock* pAncestor : pBB->GetPredecessors())
                {
                    if (pAncestor != pBB && traversed.count(pAncestor) == 0 && Contains(frontier, pAncestor) == false)
                    {
                        frontier.push_front(pAncestor);
                    }
                }

                ++it;
            }
        }
    }

    return Order;
}

void NodeOrdering::PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront)
{
    for (auto it = _Order.begin(); it != _Order.end(); ++it)
    {
        BasicBlock* pBB = *it;
        auto pos = std::distance(_Order.begin(), it);

        if (pBB->IsDivergent() && pBB->GetSuccesors().size() == 2u)
        {
            auto succ1 = std::find(_Order.begin(), _Order.end(), pBB->GetSuccesors()[0]);
            auto succ2 = std::find(_Order.begin(), _Order.end(), pBB->GetSuccesors()[1]);
            auto pos1 = std::distance(_Order.begin(), succ1);
            auto pos2 = std::distance(_Order.begin(), succ2);

            // both edges are backwards
            if (pos1 <= pos && pos2 <= pos)
            {
                BasicBlock* pVirtual = pBB->GetCFG()->NewNode(pBB->GetName() + "_VIRTUAL");
                Instruction* pTerminator = pBB->GetTerminator(); // this is a BranchCond instr

                // get branch cond info
                Instruction* pCondition = pTerminator->GetOperandInstr(0u);
                BasicBlock* pTrueSucc = pTerminator->GetOperandBB(1u);
                BasicBlock* pFalseSucc = pTerminator->GetOperandBB(2u);

                // move cond branch to Virtual node
                pVirtual->AddInstruction()->BranchCond(pCondition, pTrueSucc, pFalseSucc);

                // branch to virtual node by replacing the BranchCond with a Branch to virtual
                pTerminator->Reset()->Branch(pVirtual); 

                if (_bPutVirtualFront)
                {
                    _Order.push_front(pVirtual);
                }
                else
                {
                    // take the succuessor occuring first in the ordering
                    auto pred = pos1 < pos2 ? std::prev(succ1) : std::prev(succ2);       
                    _Order.insert(pred, pVirtual);
                }
            }
        }
    }
}

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
                BasicBlock* pVirtual = pBB->GetCFG()->NewNode(pBB->GetName() + "_VIRTORDER");
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
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

struct Front
{
    BasicBlock* pBB = nullptr;
    uint32_t uDistFromRoot = UINT32_MAX;
};

inline bool operator<(const Front& l, const Front& r) { return l.uDistFromRoot < r.uDistFromRoot; }
inline bool operator==(const Front& l, const Front& r) { return l.pBB == r.pBB; }

NodeOrder NodeOrdering::ComputeBreadthFirst(BasicBlock* _pRoot)
{
    NodeOrder Order;

    std::unordered_set<BasicBlock*> traversed;

    std::list<Front> frontier = { {_pRoot, 0u} };

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

    const auto Traverse = [&](std::list<Front>::iterator it) -> std::list<Front>::iterator
    {
        traversed.insert(it->pBB);
        Order.push_back(it->pBB);
 
        const auto InFrontier = [&](auto pSuccessor) {for (const auto& f : frontier) { if (f.pBB == pSuccessor) return true; } return false; };

        for (BasicBlock* pSuccessor : it->pBB->GetSuccesors())
        {
            if (traversed.count(pSuccessor) == 0u && InFrontier(pSuccessor) == false) // ignore backward eges / loops
            {
                frontier.push_back({ pSuccessor, it->uDistFromRoot + 1u });
            }
        }

        return frontier.erase(it);
    };

    while (frontier.empty() == false)
    {
        const size_t size = frontier.size();

        for (auto it = frontier.begin(); it != frontier.end();)
        {
            if (AncestorsTraversed(it->pBB))
            {
                it = Traverse(it);
            }
            else
            {
                ++it;
            }
        }

        // size didnt change, break tie
        if (frontier.size() == size)
        {
            Traverse(std::min_element(frontier.begin(), frontier.end()));
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

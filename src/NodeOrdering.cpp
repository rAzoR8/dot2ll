#include "NodeOrdering.h"
#include "DominatorTree.h"
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

    struct Front
    {
        BasicBlock* pBB = nullptr;
        uint32_t uDistFromRoot = UINT32_MAX;
        bool operator<(const Front& r) { return uDistFromRoot < r.uDistFromRoot; }
    };

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
            // frontier is sorted already, take the first node that is not the sink!
            for (auto it = frontier.begin(), end = frontier.end(); it != end; ++it)
            {
                if (it->pBB->IsSink() == false)
                {
                    Traverse(it);
                    break;
                }
            }
        }
    }

    return Order;
}

NodeOrder NodeOrdering::ComputePaper(BasicBlock* _pRoot, BasicBlock* _pExit)
{
    NodeOrder Order;

    DominatorTree PDT(_pExit, true);
    std::unordered_set<BasicBlock*> Visited;

    BasicBlock* A = _pRoot;
    std::deque<BasicBlock*> ToVisit = {A};

    while (ToVisit.empty() == false)
    {
        ToVisit.erase(std::remove(ToVisit.begin(), ToVisit.end(), A));
        Order.push_back(A);
        Visited.insert(A);

        for (BasicBlock* B : A->GetSuccesors())
        {
            if (Visited.count(B) == 0u)
            {
                // TODO: put them in in Dom order on the visit list
                if (std::find(ToVisit.begin(), ToVisit.end(), B) == ToVisit.end())
                {
                    ToVisit.push_back(B);
                }
            }
        }

        if (ToVisit.empty())
            break;

        std::string sNames;
        for (BasicBlock* BB : ToVisit)
        {
            sNames += ' ' + BB->GetName();
        }
        HLOG("Traversing %s open:%s", WCSTR(A->GetName()), WCSTR(sNames));

        BasicBlock* pNext = nullptr;

        for (BasicBlock* B : A->GetSuccesors())
        {
            if (Visited.count(B) != 0u)
                continue;

            pNext = B;

            for (BasicBlock* pAncestorOfA : A->GetPredecessors())
            {
                if (pAncestorOfA == A) // need to skip loops otherwise pB == pSucc triggers
                    continue;

                for (BasicBlock* pSucc : pAncestorOfA->GetSuccesors())
                {
                    if (Visited.count(pSucc) == 0) // if pB == pSucc => unvisited
                    {
                        // pSucc is an unvisited successor of an ancestor of a

                        // Do not traverse an edge E = (A, B) if B is an unvisited successor or
                        // a post-dominator of an unvisited successor of an ancestor of A (in the traversal tree?)
                        if (B == pSucc || PDT.Dominates(B, pSucc))
                        {
                            HLOG("Rejected: %s", WCSTR(B->GetName()));
                            pNext = nullptr;
                            break;
                        }
                    }
                }

                if (pNext == nullptr)
                    break;
            }

            if (pNext != nullptr)
            {
                break;
            }
        }

        if (pNext == nullptr)
        {
            pNext = ToVisit.front();
        }

        A = pNext;
    }

    return Order;
}

void NodeOrdering::ComputePaper(BasicBlock* _pA, std::unordered_set<BasicBlock*>& _Visited, const DominatorTree& _PDT, NodeOrder& _Order)
{
    _Visited.insert(_pA);
    _Order.push_back(_pA);

    HLOG("TraversalOrder: %s", WCSTR(_pA->GetName()));

    std::deque<BasicBlock*> Traverse;

    for (BasicBlock* pB : _pA->GetSuccesors())
    {
        if (_Visited.count(pB) != 0)
            continue;

        // pB is unvisited

        bool bTraverse = true;

        for (BasicBlock* pAncestorOfA : _pA->GetPredecessors())
        {            
            if (pAncestorOfA == _pA) // need to skip loops otherwise pB == pSucc triggers
                continue;

            for (BasicBlock* pSucc : pAncestorOfA->GetSuccesors())
            {
                if (_Visited.count(pSucc) == 0) // if pB == pSucc => unvisited
                {
                    // pSucc is an unvisited successor of an ancestor of a
                   
                    // Do not traverse an edge E = (A, B) if B is an unvisited successor or
                    // a post-dominator of an unvisited successor of an ancestor of A (in the traversal tree?)
                    if (pB == pSucc || _PDT.Dominates(pB, pSucc))
                    {
                        HLOG("Rejected: %s", WCSTR(pB->GetName()));
                        bTraverse = false;
                        break;
                    }
                }
            }
            if (bTraverse == false)
                break;
        }

        if (bTraverse)
        {
            // When choosing between two successors of block B, visit the one which is NOT a post dominator of B first
            if (_PDT.Dominates(pB, _pA))
            {
                Traverse.push_back(pB);
            }
            else
            {
                Traverse.push_front(pB);
            }            
        }
    }

    for (BasicBlock* pB : Traverse)
    {
        ComputePaper(pB, _Visited, _PDT, _Order);
    }
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

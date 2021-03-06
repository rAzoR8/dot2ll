#include "NodeOrdering.h"
#include "Function.h"
#include "CFGUtils.h"
#include "hlx/include/StringHelpers.h"

#include <deque>
#include <unordered_set>

NodeOrder NodeOrdering::Custom(ControlFlowGraph& _CFG, const std::string& _sCustomOrdering)
{
    NodeOrder Order;

    for (const auto& sName : hlx::split(_sCustomOrdering, ','))
    {
        BasicBlock* pBB = _CFG.FindNode(sName);
        if (pBB != nullptr)
        {
            Order.push_back(pBB);
        }
        else
        {
            HERROR("Block %s not found in CFG", WCSTR(sName));
            break;
        }
    }

    if (Order.size() != _CFG.GetNodes().size() - 1)
    {
        HERROR("Incomplete custom ordering %s for function with %d basic blocks", WCSTR(_sCustomOrdering), (uint32_t)_CFG.GetNodes().size());
    }

    return Order;
}

NodeOrder NodeOrdering::DepthFirst(BasicBlock* _pRoot, const bool _bExitLast)
{
    NodeOrder Order;
    BasicBlock* pExit = nullptr;

    for (BasicBlock* pBB : CFGUtils::DepthFirst(_pRoot))
    {
        if (pBB->IsSink() && _bExitLast)
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

NodeOrder NodeOrdering::PostOrderTraversal(BasicBlock* _pRoot, const bool _bReverse)
{
    return CFGUtils::PostOrderTraversal(_pRoot, _bReverse);
}

void DominanceRegionImpl(DominatorTreeNode* _pNode, NodeOrder& _Order)
{
    _Order.push_back(_pNode->GetBasicBlock());

    DominatorTreeNode* pExitBranch = nullptr;

    for (DominatorTreeNode* pChild : _pNode->GetChildren())
    {
        if (pChild->ExitAttached())
        {
            pExitBranch = pChild;
            continue;
        }

        DominanceRegionImpl(pChild, _Order);
    }

    // traverse exit branch last
    if (pExitBranch != nullptr)
    {
        DominanceRegionImpl(pExitBranch, _Order);
    }
}

NodeOrder NodeOrdering::DominanceRegion(BasicBlock* _pRoot)
{
    NodeOrder Order;

    DominatorTree DT = DominatorTree(_pRoot);

    DominanceRegionImpl(DT.GetRootNode(), Order);

    return Order;
}

NodeOrder NodeOrdering::BreadthFirst(BasicBlock* _pRoot, const bool _bCheckDominance)
{
    DominatorTree PDT;

    if (_bCheckDominance)
    {
        PDT = std::move(_pRoot->GetCFG()->GetFunction()->GetPostDominatorTree());
    }

    NodeOrder Order;

    std::unordered_set<BasicBlock*> traversed;

    struct Front
    {
        BasicBlock* pBB = nullptr;
        uint32_t uDistFromRoot = UINT32_MAX;
        bool operator<(const Front& r) { return uDistFromRoot < r.uDistFromRoot; }
    };

    std::list<Front> frontier = { {_pRoot, 0u} };

    const auto Traverse = [&](std::list<Front>::iterator it) -> std::list<Front>::iterator
    {
        HLOG("Traversed %s", WCSTR(it->pBB->GetName()));

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

    const auto Pick = [&]()->std::list<Front>::iterator
    {
        if (frontier.size() == 1u)
            return frontier.begin();

        if (_bCheckDominance)
        {
            BasicBlock* pPrev = Order.back();

            // pick the successor which does not dominate the previous node
            for (auto it = frontier.begin(); it != frontier.end(); ++it)
            {
                BasicBlock* pCur = it->pBB;
                if (pCur->IsSuccessorOf(pPrev) && PDT.Dominates(pCur, pPrev) == false)
                {
                    return it;
                }
            }
        }

        // pick the one with the shortest path which is not the sink
        return frontier.front().pBB->IsSink() ? std::next(frontier.begin()) : frontier.begin();
    };

    while (frontier.empty() == false)
    {
        Traverse(Pick());
    }

    return Order;
}

NodeOrder NodeOrdering::DepthFirstPostDom(BasicBlock* _pRoot, BasicBlock* _pExit)
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
            if (ToVisit.size() > 1)
            {
                // TODO: pick successor of A which is not the IPDOM
                for (BasicBlock* BB : ToVisit)
                {
                    if (BB->IsSink() == false)
                    {
                        pNext = BB;
                        break;
                    }
                }
            }
            else
            {
                pNext = ToVisit.front();
            }
        }

        A = pNext;
    }

    return Order;
}

bool NodeOrdering::PrepareOrdering(NodeOrder& _Order, const bool _bPutVirtualFront, const bool _bExitLast)
{
    bool bChanged = false;
   
    if (_bExitLast && _Order.empty() == false && _Order.back()->IsSink() == false)
    {
        if (auto it = std::find_if(_Order.begin(), _Order.end(), [](BasicBlock* pBB) {return pBB->IsSink(); }); it != _Order.end())
        {
            BasicBlock* pExit = *it;
            _Order.erase(it);
            _Order.push_back(pExit);

            HLOG("Enforcing exit block last");
            //bChanged = true;
        }        
    }

    for (auto it = _Order.begin(); it != _Order.end(); ++it)
    {
        BasicBlock* pBB = *it;
        auto pos = std::distance(_Order.begin(), it);

        if (pBB->IsDivergent())
        {
            auto succ1 = std::find(_Order.begin(), _Order.end(), pBB->GetSuccesors()[0]);
            auto succ2 = std::find(_Order.begin(), _Order.end(), pBB->GetSuccesors()[1]);
            auto pos1 = std::distance(_Order.begin(), succ1);
            auto pos2 = std::distance(_Order.begin(), succ2);

            // both edges are backwards
            if (pos1 <= pos && pos2 <= pos)
            {
                BasicBlock* pVirtual = pBB->GetCFG()->NewNode(pBB->GetName() + "_VIRTUAL");
                pVirtual->SetDivergent();

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
                    HLOG("Inserting %s before %s in ordering", WCSTR(pVirtual->GetName()), WCSTR(_Order.front()->GetName()));
                    _Order.push_front(pVirtual);
                }
                else
                {
                    // take the succuessor occuring first in the ordering  
                    auto it = pos1 < pos2 ? succ1 : succ2;
                    HLOG("Inserting %s before %s in ordering", WCSTR(pVirtual->GetName()), WCSTR((*it)->GetName()));
                    _Order.insert(pos1 < pos2 ? succ1 : succ2, pVirtual);
                }

                bChanged = true;
            }
        }
    }

    return bChanged;
}

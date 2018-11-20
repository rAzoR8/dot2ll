#pragma once

#include "ControlFlowGraph.h"
#include <vector>
#include <unordered_set>

struct CFGUtils
{
    static BasicBlock* GetSource(ControlFlowGraph& _Graph)
    {
        for (BasicBlock& node : _Graph)
        {
            if (node.GetPredecessors().empty())
            {
                return &node;
            }
        }

        return nullptr;
    }

    static const BasicBlock* GetSource(const ControlFlowGraph& _Graph)
    {
        for (const BasicBlock& node : _Graph)
        {
            if (node.GetPredecessors().empty())
            {
                return &node;
            }
        }

        return nullptr;
    }

    // makes sure source node comes first
    template <class TCFG>
    static decltype(auto) GetSerializationOrder(TCFG& _Graph)
    {
        using NodeType = decltype(GetSource(_Graph));

        std::vector<NodeType> Nodes;

        NodeType pSource = GetSource(_Graph);

        if (pSource != nullptr)
        {
            Nodes.push_back(pSource);

            for (auto& node : _Graph)
            {
                if (&node != pSource)
                {
                    Nodes.push_back(&node);
                }
            }
        }

        return Nodes;
    }

    // check if there is a path from _pCurrent to _pNode
    template <class BB = BasicBlock>
    static bool IsReachable(const BB* _pNode, const BB* _pCurrent, std::unordered_set<const BB*>& _Set, const bool _bForward = true, const BB* _pWithout = nullptr)
    {
        if (_pCurrent == _pNode)
            return true;

        const auto& Successors = _bForward ? _pCurrent->GetSuccesors() : _pCurrent->GetPredecessors();

        for (const BB* pSucc : Successors)
        {
            if (pSucc == _pWithout)
                continue;

            if (_Set.count(pSucc) == 0)
            {
                _Set.insert(pSucc);
                if (IsReachable(_pNode, pSucc, _Set, _bForward, _pWithout))
                {
                    return true;
                }
            }
        }

        return false;
    }

    template <class BB = BasicBlock>
    static void DepthFirst(BB* _pRoot, std::vector<BB*>& _OutVec, std::unordered_set<BB*>& _Set, const bool _bForward = true)
    {
        if (_pRoot == nullptr || _Set.count(_pRoot) != 0u)
            return;

        _Set.insert(_pRoot);
        _OutVec.push_back(_pRoot);

        const auto& Successors = _bForward ? _pRoot->GetSuccesors() : _pRoot->GetPredecessors();

        for (BB* pSucc : Successors)
        {
            DepthFirst(pSucc, _OutVec, _Set, _bForward);
        }
    }

    template <class BB = BasicBlock>
    static std::vector<BB*> DepthFirst(BB* _pRoot, const bool _bForward = true)
    {
        std::unordered_set<BB*> Set;
        std::vector<BB*> Vec;
        DepthFirst(_pRoot, Vec, Set, _bForward);

        return Vec;
    }

    template <class BB = BasicBlock>
    static void PostOrderTraversal(std::list<BB*>& _Order, std::unordered_set<BB*>& _Visited, BB* _pBB, const bool _bReverse)
    {
        _Visited.insert(_pBB);

        for (BB* pSucc : _pBB->GetSuccesors())
        {
            if (_Visited.count(pSucc) == 0)
            {
                PostOrderTraversal(_Order, _Visited, pSucc, _bReverse);
            }
        }

        if (_bReverse)
        {
            _Order.push_front(_pBB);
        }
        else
        {
            _Order.push_back(_pBB);
        }
    }

    template <class BB = BasicBlock>
    static std::list<BB*> PostOrderTraversal(BB* _pRoot, const bool _bReverse)
    {
        std::list<BB*> Order;
        std::unordered_set<BB*> Visited;

        PostOrderTraversal(Order, Visited, _pRoot, _bReverse);
        return Order;
    }
};
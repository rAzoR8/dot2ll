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

    static bool IsReachable(const BasicBlock* _pNode, const BasicBlock* _pCurrent, std::unordered_set<const BasicBlock*>& _Set, const bool _bForward = true, const BasicBlock* _pWithout = nullptr)
    {
        if (_pCurrent == _pNode)
            return true;

        const auto& Successors = _bForward ? _pCurrent->GetSuccesors() : _pCurrent->GetPredecessors();

        for (const BasicBlock* pSucc : Successors)
        {
            if (pSucc == _pWithout)
                continue;

            if (_Set.count(pSucc) == 0)
            {
                _Set.insert(pSucc);
                if (IsReachable(_pNode, pSucc, _Set, _bForward))
                {
                    return true;
                }
            }
        }

        return false;
    }

    static void DepthFirst(const BasicBlock* _pRoot, std::vector<const BasicBlock*>& _OutVec, std::unordered_set<const BasicBlock*>& _Set, const bool _bForward = true)
    {
        if (_pRoot == nullptr)
            return;

        _OutVec.push_back(_pRoot);

        const auto& Successors = _bForward ? _pRoot->GetSuccesors() : _pRoot->GetPredecessors();

        for (const BasicBlock* pSucc : Successors)
        {
            if (_Set.count(pSucc) == 0)
            {
                _Set.insert(pSucc);
                DepthFirst(pSucc, _OutVec, _Set, _bForward);
            }
        }
    }

    static std::vector<const BasicBlock*> DepthFirst(const BasicBlock* _pRoot, const bool _bForward = true)
    {
        std::unordered_set<const BasicBlock*> Set;
        std::vector<const BasicBlock*> Vec;
        DepthFirst(_pRoot, Vec, Set);

        return Vec;
    }
};
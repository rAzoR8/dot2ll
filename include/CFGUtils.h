#pragma once

#include "ControlFlowGraph.h"
#include <vector>

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
};
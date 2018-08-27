#pragma once

//#include "ControlFlowGraph.h"
#include <vector>

struct CFGUtils
{
    template <class TCFG, class NodeType = typename TCFG::NodeType>
    static NodeType* GetSource(TCFG& _Graph)
    {
        for (NodeType& node : _Graph)
        {
            if (node.GetPredecessors().empty())
            {
                return &node;
            }
        }

        return nullptr;
    }

    template <class TCFG, class NodeType = typename TCFG::NodeType>
    static const NodeType* GetSource(const TCFG& _Graph)
    {
        for (const NodeType& node : _Graph)
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
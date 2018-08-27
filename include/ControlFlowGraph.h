#pragma once

#include "BasicBlock.h"
#include <unordered_map>

template <class TNodeType = DefaultBB>
class ControlFlowGraph
{
public:
    using Nodes = std::vector<TNodeType>;

    ControlFlowGraph(const size_t _uMaxNodes = 128u)
    {
        m_Nodes.reserve(_uMaxNodes);
    };

    ControlFlowGraph(ControlFlowGraph&& _Other) :
        m_Nodes(std::move(_Other.m_Nodes)), m_NodeMap(std::move(_Other.m_NodeMap)) {}

    ~ControlFlowGraph() {};

    template <class ...Args>
    TNodeType* AddNode(const size_t uIdentifier, Args&& ..._Args)
    {
        if (auto it = m_NodeMap.find(uIdentifier); it != m_NodeMap.end())
        {
            return it->second;
        }

        TNodeType* pNode = &m_Nodes.emplace_back(std::forward<Args>(_Args)...);
        m_NodeMap.insert({ uIdentifier, pNode });
        return pNode;
    }

    const Nodes& GetNodes() const { return m_Nodes; }
    Nodes& GetNodes() { return m_Nodes; }

    typename Nodes::iterator begin() noexcept { return m_Nodes.begin(); }
    typename Nodes::iterator end() noexcept { return m_Nodes.end(); }

    typename Nodes::const_iterator begin() const noexcept { return m_Nodes.begin(); }
    typename Nodes::const_iterator end() const noexcept { return m_Nodes.end(); }

private:
    Nodes m_Nodes;

    std::unordered_map<size_t, TNodeType*> m_NodeMap;
};

using DefaultCFG = ControlFlowGraph<>;

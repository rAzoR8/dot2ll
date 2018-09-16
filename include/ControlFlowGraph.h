#pragma once

#include "BasicBlock.h"
#include <unordered_map>

class ControlFlowGraph
{
    friend class BasicBlock;
public:
    using Nodes = std::vector<BasicBlock>;

    ControlFlowGraph(const size_t _uMaxNodes = 128u)
    {
        m_Nodes.reserve(_uMaxNodes);
    };

    ControlFlowGraph(ControlFlowGraph&& _Other) :
        m_Nodes(std::move(_Other.m_Nodes)),
        m_Instructions(std::move(_Other.m_Instructions)),
        m_NodeMap(std::move(_Other.m_NodeMap)) {}

    ~ControlFlowGraph() {};

    // use name hash as identifier
    BasicBlock* AddNode(const std::string& _sName);
    BasicBlock* AddNode(const uint64_t uIdentifier, const std::string& _sName = {});
    BasicBlock* GetNode(const uint64_t uIdentifier) const;

    const Nodes& GetNodes() const { return m_Nodes; }
    Nodes& GetNodes() { return m_Nodes; }

    typename Nodes::iterator begin() noexcept { return m_Nodes.begin(); }
    typename Nodes::iterator end() noexcept { return m_Nodes.end(); }

    typename Nodes::const_iterator begin() const noexcept { return m_Nodes.begin(); }
    typename Nodes::const_iterator end() const noexcept { return m_Nodes.end(); }

    TypeInfo ResolveType(const uint64_t _uTypeId) const;
    TypeInfo ResolveType(const Instruction* _pType) const;

    //Instruction* GetInstruction(const uint64_t _uId);
    Instruction* GetInstruction(const uint64_t _uId) const;

private:
    Nodes m_Nodes;

    std::vector<Instruction*> m_Instructions;
    std::unordered_map<uint64_t, BasicBlock*> m_NodeMap;
};
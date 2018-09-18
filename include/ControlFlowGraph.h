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
        m_NodeIdentifierMap(std::move(_Other.m_NodeIdentifierMap)) {}

    ~ControlFlowGraph() {};

    // always creates a new node
    BasicBlock* NewNode(const std::string& _sName = {});

    const Nodes& GetNodes() const { return m_Nodes; }
    Nodes& GetNodes() { return m_Nodes; }

    typename Nodes::iterator begin() noexcept { return m_Nodes.begin(); }
    typename Nodes::iterator end() noexcept { return m_Nodes.end(); }

    typename Nodes::const_iterator begin() const noexcept { return m_Nodes.begin(); }
    typename Nodes::const_iterator end() const noexcept { return m_Nodes.end(); }

    TypeInfo ResolveType(const InstrId _uTypeId) const;
    TypeInfo ResolveType(const Instruction* _pType) const;

    //Instruction* GetInstruction(const uint64_t _uId);
    Instruction* GetInstruction(const InstrId _uId) const { return _uId < m_Instructions.size() ? m_Instructions[_uId] : nullptr; };

    BasicBlock* FindNode(const std::string& _sName);
    const BasicBlock* FindNode(const std::string& _sName) const;

    BasicBlock* GetNode(const InstrId _uId) { return _uId < m_Nodes.size() ? &m_Nodes[_uId] : nullptr; };
    const BasicBlock* GetNode(const InstrId _uId) const { return _uId < m_Nodes.size() ? &m_Nodes[_uId] : nullptr; };

private:
    Nodes m_Nodes;

    std::vector<Instruction*> m_Instructions;
    // name hash -> index into nodes
    std::unordered_map<uint64_t, InstrId> m_NodeIdentifierMap;
};
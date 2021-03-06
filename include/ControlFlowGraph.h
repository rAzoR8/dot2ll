#pragma once

#include "BasicBlock.h"
#include <unordered_map>

class ControlFlowGraph
{
    friend class BasicBlock;
    friend class Function;

public:
    using Nodes = std::vector<BasicBlock>;

    ControlFlowGraph(Function* _pParent = nullptr, const size_t _uMaxNodes = 128u);

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

    typename Nodes::reverse_iterator rbegin() noexcept { return m_Nodes.rbegin(); }
    typename Nodes::reverse_iterator rend() noexcept { return m_Nodes.rend(); }

    typename Nodes::const_reverse_iterator rbegin() const noexcept { return m_Nodes.rbegin(); }
    typename Nodes::const_reverse_iterator rend() const noexcept { return m_Nodes.rend(); }

    TypeInfo ResolveType(const InstrId _uTypeId) const;
    TypeInfo ResolveType(const Instruction* _pType) const;

    Instruction* GetInstruction(const InstrId _uId) const { return _uId < m_Instructions.size() ? m_Instructions[_uId] : nullptr; };

    BasicBlock* FindNode(const std::string& _sName);
    const BasicBlock* FindNode(const std::string& _sName) const;

    BasicBlock* GetNode(const InstrId _uId) { return _uId < m_Nodes.size() ? &m_Nodes[_uId] : nullptr; };
    const BasicBlock* GetNode(const InstrId _uId) const { return _uId < m_Nodes.size() ? &m_Nodes[_uId] : nullptr; };

    Function* GetFunction() { return m_pFunction; }
    const Function* GetFunction() const { return m_pFunction; }

private:
    Function* m_pFunction;
    Nodes m_Nodes;

    std::vector<Instruction*> m_Instructions;
    // name hash -> index into nodes
    std::unordered_map<uint64_t, InstrId> m_NodeIdentifierMap;
};
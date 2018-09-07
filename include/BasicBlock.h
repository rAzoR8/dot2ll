#pragma once

#include "Instruction.h"
#include <deque>
#include <string>

class BasicBlock
{
    friend class Instruction;
    friend class ControlFlowGraph;

    BasicBlock(const uint64_t _uIndentifier, ControlFlowGraph* _pParent, const std::string& _sName) :
        m_uIdentifier(_uIndentifier), m_pParent(_pParent), m_sName(_sName)
    {        
    }

public:
    using Successors = std::vector<BasicBlock*>;
    using Predecessors = std::vector<BasicBlock*>;
    using Instructions = std::deque<Instruction>;

    ~BasicBlock() {};

    const Instructions& GetInstructions() const { return m_Instructions; }
    const Successors& GetSuccesors() const { return m_Successors; }
    const Predecessors& GetPredecessors() const { return m_Predecessors; }

    ControlFlowGraph* GetCFG() const { return m_pParent; }

    Instruction* AddInstruction();
    Instruction* AddInstructionFront();

    Instruction* GetTerminator();

    typename Instructions::iterator begin() noexcept { return m_Instructions.begin(); }
    typename Instructions::iterator end() noexcept { return m_Instructions.end(); }

    typename Instructions::const_iterator begin() const noexcept { return m_Instructions.begin(); }
    typename Instructions::const_iterator end() const noexcept { return m_Instructions.end(); }

    const std::string& GetName() const { return m_sName; }

    const bool IsSource() const { return m_bSource; }
    const bool IsSink() const { return m_bSink; }

    const uint64_t& GetIdentifier() const { return m_uIdentifier; }

    void SetDivergent(const bool _bDivergent) { m_bDivergent = _bDivergent; }
    const bool IsDivergent() const { return m_bDivergent; }

private:
    const uint64_t m_uIdentifier;

    std::string m_sName; // Label
    bool m_bSource = true; // Entry
    bool m_bSink = true; // Exit
    bool m_bDivergent = false; // non uniform

    Instructions m_Instructions;
    Successors m_Successors;
    Predecessors m_Predecessors;
    ControlFlowGraph* const m_pParent;

    Instruction* m_pTerminator = nullptr;
};

inline Instruction* BasicBlock::GetTerminator()
{
    if (m_pTerminator != nullptr)
    {
        return m_pTerminator;
    }

    if (m_Instructions.empty() == false)
    {
        return &m_Instructions.back();
    }

    return nullptr;
}
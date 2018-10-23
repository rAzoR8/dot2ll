#pragma once

#include "Instruction.h"
#include <deque>
#include <string>

class BasicBlock
{
    friend class Instruction;
    friend class ControlFlowGraph;
    friend class Function;

public:
    using Vec = std::vector<BasicBlock*>;
    using Instructions = std::deque<Instruction>;

    BasicBlock(const InstrId _uIndentifier, ControlFlowGraph* _pParent, const std::string& _sName) :
        m_uIdentifier(_uIndentifier), m_pParent(_pParent), m_sName(_sName)
    {
    }

    BasicBlock(BasicBlock&& _Other);
    BasicBlock(const BasicBlock&) = delete;

    ~BasicBlock() {};

    const Instructions& GetInstructions() const { return m_Instructions; }
    const Vec& GetSuccesors() const { return m_Successors; }
    const Vec& GetPredecessors() const { return m_Predecessors; }

    ControlFlowGraph* GetCFG() const { return m_pParent; }

    Instruction* AddInstruction();
    Instruction* AddInstructionFront();

    Instruction* GetTerminator() { return m_pTerminator; }
    const Instruction* GetTerminator() const { return m_pTerminator; }

    typename Instructions::iterator begin() noexcept { return m_Instructions.begin(); }
    typename Instructions::iterator end() noexcept { return m_Instructions.end(); }

    typename Instructions::const_iterator begin() const noexcept { return m_Instructions.begin(); }
    typename Instructions::const_iterator end() const noexcept { return m_Instructions.end(); }

    const std::string& GetName() const { return m_sName; }

    const bool IsSource() const { return m_bSource; }
    const bool IsSink() const { return m_bSink; }
    const bool IsVirtual() const { return m_bVirtual; }

    const InstrId& GetIdentifier() const { return m_uIdentifier; }

    void SetDivergent(const bool _bDivergent) { m_bDivergent = _bDivergent; }
    const bool IsDivergent() const { return m_bDivergent && m_Successors.size() > 1u; }
    const bool GetDivergenceQualifier() const { return m_bDivergent; }
    void SetVirtual(const bool _bVirtual) { m_bVirtual = _bVirtual; }
private:
    const InstrId m_uIdentifier;

    std::string m_sName; // Label
    bool m_bSource = true; // Entry
    bool m_bSink = true; // Exit
    bool m_bDivergent = true; // non uniform by default
    bool m_bVirtual = false; // virtual entry or exit node

    Instructions m_Instructions;
    Vec m_Successors;
    Vec m_Predecessors;
    ControlFlowGraph* /*const*/ m_pParent;

    Instruction* m_pTerminator = nullptr;
};
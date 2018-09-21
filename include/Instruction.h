#pragma once

#include "InstructionDefines.h"

class Instruction
{
    friend class BasicBlock;
    friend class Function;    

public:
    Instruction(const InstrId _uIdentifier, BasicBlock* _pParent) :
        uIdentifier(_uIdentifier), pParent(_pParent), sAlias(std::to_string(_uIdentifier)) {}

    Instruction(const Instruction&) = delete;
    Instruction(Instruction&&) = delete;    
    Instruction& operator=(const Instruction& _Other) = delete;
    Instruction& operator=(Instruction&&) = delete;

    ~Instruction() {};    

    const std::vector<Operand>& GetOperands() const { return Operands; }
    const std::vector<Decoration> GetDecorations() const { return Decorations; }

    const bool Is(const EDecoration _kDecoration) const;

    const EInstruction GetInstruction() const { return kInstruction; }
    const InstrId& GetIdentifier() const { return uIdentifier; }
    const InstrId& GetResultTypeId() const { return uResultTypeId; }

    void SetAlias(const std::string& _sAlias) { sAlias = _sAlias; };
    const std::string& GetAlias() const { return sAlias; }

    // all instruction generators return their pointers if construction was successful, nullptr other wise
    Instruction* Nop();

    // creates a new decoration for this instruction
    void Decorate(const std::vector<Decoration>& _Decorations);
    void StringDecoration(const std::string& _sString);
    std::string GetStringDecoration() const;

    Instruction* Constant(const Instruction* _pType, const std::vector<InstrId>& _ConstantData);

    Instruction* Equal(const Instruction* _pLeft, const Instruction* _pRight);

    Instruction* Return(const Instruction* _pResult = nullptr);
    Instruction* Branch(BasicBlock* _pTarget);
    Instruction* BranchCond(const Instruction* _pCondtion, BasicBlock* _pTrueTarget, BasicBlock* _pFalseTarget);

    Instruction* Reset();

    Instruction* GetOperandInstr(const InstrId _uIndex) const;
    BasicBlock* GetOperandBB(const InstrId _uIndex) const;

private:
    // to be called from function
    Instruction* FunctionParameter(const Instruction* _pType, const InstrId _uIndex);
    Instruction* Type(const EType _kType, const uint32_t _uElementBits, const uint32_t _uElementCount, const std::vector<InstrId>& _SubTypes = {}, const std::vector<Decoration>& _Decorations = {});

private:
    EInstruction kInstruction = kInstruction_Undefined; // opcode identifier
    const InstrId uIdentifier; // result identifier
    BasicBlock* const pParent;
    std::string sAlias;

    InstrId uResultTypeId = InvalidId;
    std::vector<Operand> Operands; // operand identifiers
    std::vector<Decoration> Decorations;

};

#ifndef CHECK_INSTR
#define CHECK_INSTR if(kInstruction != kInstruction_Undefined || pParent->m_pTerminator != nullptr) return nullptr;
#endif

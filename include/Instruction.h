#pragma once

#include "InstructionDefines.h"

class Instruction
{
    friend class BasicBlock;
    friend class Function;
    
    Instruction(const uint64_t _uIdentifier, BasicBlock* _pParent) :
        uIdentifier(_uIdentifier), pParent(_pParent), sAlias(std::to_string(_uIdentifier)) {}

public:
    ~Instruction() {};    

    // all instruction generators return their pointers if construction was successful, nullptr other wise
    Instruction* Nop();

    // creates a new decoration for this instruction
    void Decorate(const std::vector<Decoration>& _Decorations);
    void StringDecoration(const std::string& _sString);
    std::string GetStringDecoration() const;

    Instruction* Constant(const Instruction* _pType, const std::vector<uint64_t>& _ConstantData);

    Instruction* Equal(const Instruction* _pLeft, const Instruction* _pRight);

    Instruction* Return(const Instruction* _pResult = nullptr);
    Instruction* Branch(BasicBlock* _pTarget);
    Instruction* BranchCond(const Instruction* _pCondtion, BasicBlock* _pTrueTarget, BasicBlock* _pFalseTarget);

    const std::vector<Operand>& GetOperands() const { return Operands; }
    const std::vector<Decoration> GetDecorations() const { return Decorations; }

    const bool Is(const EDecoration _kDecoration) const;

    const EInstruction GetInstruction() const { return kInstruction; }
    const uint64_t& GetIdentifier() const { return uIdentifier; }
    const uint64_t& GetResultTypeId() const { return uResultTypeId; }

    void SetAlias(const std::string& _sAlias) { sAlias = _sAlias; };
    const std::string& GetAlias() const { return sAlias; }

private:
    // to be called from function
    Instruction* FunctionParameter(const Instruction* _pType, const uint64_t _uIndex);
    Instruction* Type(const EType _kType, const uint32_t _uElementBits, const uint32_t _uElementCount, const std::vector<uint64_t>& _SubTypes = {}, const std::vector<Decoration>& _Decorations = {});

    //Instruction* Reset();

private:
    const uint64_t uIdentifier; // result identifier
    BasicBlock* const pParent;
    std::string sAlias;

    EInstruction kInstruction = kInstruction_Undefined; // opcode identifier
    uint64_t uResultTypeId = InvalidId;
    std::vector<Operand> Operands; // operand identifiers
    std::vector<Decoration> Decorations;

};

#ifndef CHECK_INSTR
#define CHECK_INSTR if(kInstruction != kInstruction_Undefined || pParent->m_pTerminator != nullptr) return nullptr;
#endif

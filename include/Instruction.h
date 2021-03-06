#pragma once

#include "InstructionDefines.h"
#include "hlx/include/Logger.h"
#undef S

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

    bool operator==(const Instruction& _Other) const { return uIdentifier == _Other.uIdentifier && pParent == _Other.pParent; }
    bool operator!=(const Instruction& _Other) const { return uIdentifier != _Other.uIdentifier || pParent != _Other.pParent; }

    ~Instruction() {};    

    const std::vector<Operand>& GetOperands() const { return Operands; }
    const std::vector<Decoration> GetDecorations() const { return Decorations; }

    const bool Is(const EDecoration _kDecoration) const;
    const bool Is(const EInstruction _kInstr) const { return kInstruction == _kInstr; };

    const EInstruction GetInstruction() const { return kInstruction; }
    const InstrId& GetIdentifier() const { return uIdentifier; }
    const InstrId& GetResultTypeId() const { return uResultTypeId; }

    void SetAlias(const std::string& _sAlias) { sAlias = _sAlias; };
    const std::string& GetAlias() const { return sAlias; }

    Instruction* GetPrevInstruction() const;
    Instruction* GetNextInstruction() const;

    // all instruction generators return their pointers if construction was successful, nullptr other wise
    Instruction* Nop();

    // creates a new decoration for this instruction
    void Decorate(const std::vector<Decoration>& _Decorations);
    void StringDecoration(const std::string& _sString);
    std::string GetStringDecoration() const;

    Instruction* Equal(const Instruction* _pLeft, const Instruction* _pRight);

    Instruction* Return(const Instruction* _pResult = nullptr);
    Instruction* Branch(BasicBlock* _pTarget);
    Instruction* BranchCond(const Instruction* _pCondtion, BasicBlock* _pTrueTarget, BasicBlock* _pFalseTarget);

    Instruction* Phi(const std::vector<Instruction*>& _Value, const std::vector<BasicBlock*>& _Origins);

    Instruction* Not(Instruction* _pValue);

    Instruction* Reset();

    Instruction* GetOperandInstr(const InstrId _uIndex) const;
    BasicBlock* GetOperandBB(const InstrId _uIndex) const;

    BasicBlock* GetBasicBlock() const { return pParent; }

private:
    // to be called from function
    Instruction* FunctionParameter(const Instruction* _pType, const InstrId _uIndex);
    Instruction* Type(const EType _kType, const uint32_t _uElementBits, const uint32_t _uElementCount, const std::vector<InstrId>& _SubTypes = {}, const std::vector<Decoration>& _Decorations = {});
    Instruction* Constant(const Instruction* _pType, const std::vector<InstrId>& _ConstantData);

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
#define CHECK_INSTR if(kInstruction != kInstruction_Undefined) {HFATAL("Invalid instruction state"); return nullptr;}
#endif
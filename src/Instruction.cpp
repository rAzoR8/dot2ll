#include "Instruction.h"
#include "ControlFlowGraph.h"

Instruction* Instruction::Nop()
{
    CHECK_INSTR;
    kInstruction = kInstruction_Nop;
    return this;
}

Instruction* Instruction::Return(const Instruction* _pResult)
{
    CHECK_INSTR;

    kInstruction = kInstruction_Return;

    if (_pResult != nullptr)
    {
        Operands.emplace_back(kOperandType_InstructionId, _pResult->uIdentifier);
        uResultTypeId = _pResult->uResultTypeId;
    }

    pParent->m_pTerminator = this;
    pParent->m_bSink = true;

    return this;
}

void Instruction::Decorate(const std::vector<Decoration>& _Decorations)
{
    Decorations.insert(Decorations.end(), _Decorations.begin(), _Decorations.end());
}

void Instruction::StringDecoration(const std::string& _sName)
{
    char* pChar = nullptr;

    uint32_t i = 0;
    for (const char& c : _sName)
    {
        if (i++ % 4 == 0)
        {
            pChar = reinterpret_cast<char*>(&Decorations.emplace_back(kDecoration_Name, 0u).uUserData);
        }

        *pChar = c;
        pChar++;
    }
}

std::string Instruction::GetStringDecoration() const
{
    std::string sDecoration;

    for (const Decoration& d : Decorations)
    {
        if (d.kType == kDecoration_Name)
        {
            for (uint32_t i = 0u; i < 4; ++i)
            {
                const char c = reinterpret_cast<const char*>(&d.uUserData)[i];
                if (c != 0)
                {
                    sDecoration += c;
                }
            }
        }
    }

    return sDecoration;
}

Instruction& Instruction::operator=(const Instruction & _Other)
{
    kInstruction = _Other.kInstruction;
    Operands = _Other.Operands;
    Decorations = _Other.Decorations;
    sAlias = _Other.sAlias;
    uResultTypeId = _Other.uResultTypeId;
    return *this;
}

const bool Instruction::Is(const EDecoration _kDecoration) const
{
    for (const Decoration& d : Decorations)
    {
        if (d.kType == _kDecoration)
            return true;
    }

    return false;
}

Instruction* Instruction::FunctionParameter(const Instruction* _pType, const uint64_t _uIndex)
{
    CHECK_INSTR;

    if (_pType != nullptr)
    {
        kInstruction = kInstruction_FunctionParameter;
        Operands.emplace_back(kOperandType_Constant, _uIndex);
        uResultTypeId = _pType->uIdentifier;
        return this;
    }

    return nullptr;
}

Instruction* Instruction::Constant(const Instruction* _pType, const std::vector<uint64_t>& _ConstantData)
{
    CHECK_INSTR;

    if (_pType == nullptr)
    {
        return nullptr;
    }

    kInstruction = kInstruction_Constant;
    uResultTypeId = _pType->uIdentifier;

    for (const uint64_t& data : _ConstantData)
    {
        Operands.emplace_back(kOperandType_Constant, data);
    }

    return this;
}

Instruction* Instruction::Type(const EType _kType, const uint32_t _uElementBits, const uint32_t _uElementCount, const std::vector<uint64_t>& _SubTypes, const std::vector<Decoration>& _Decorations)
{
    CHECK_INSTR;

    kInstruction = kInstruction_Type;

    Operands.emplace_back(kOperandType_Constant, static_cast<uint64_t>(_kType));

    switch (_kType)
    {
        // nothing to do
    case kType_Void:
    case kType_Bool:
        break;
    case kType_Int:
    case kType_UInt:
    case kType_Float:
        Operands.emplace_back(kOperandType_Constant, static_cast<uint64_t>(_uElementBits));
        Operands.emplace_back(kOperandType_Constant, static_cast<uint64_t>(_uElementCount));
        break;
    case kType_Pointer:
        Operands.emplace_back(kOperandType_InstructionId, _SubTypes.front());
        break;
    case kType_Array:
        Operands.emplace_back(kOperandType_Constant, static_cast<uint64_t>(_uElementCount));
        Operands.emplace_back(kOperandType_InstructionId, _SubTypes.front());
        break;
    case kType_Struct:
        for (const uint64_t& type : _SubTypes)
        {
            Operands.emplace_back(kOperandType_InstructionId, type);
        }
        break;
    default:
        break;
    }

    Decorate(_Decorations);

    return this;
}

Instruction* Instruction::Reset()
{
    kInstruction = kInstruction_Undefined;
    uResultTypeId = InvalidId;
    Operands.clear();
    Decorations.clear();

    return this;
}

Instruction* Instruction::Equal(const Instruction* _pLeft, const Instruction* _pRight)
{
    CHECK_INSTR;

    if (_pLeft != nullptr && _pRight != nullptr)
    {
        if (_pLeft->uResultTypeId == _pRight->uResultTypeId)
        {
            kInstruction = kInstruction_Equal;
            Operands.emplace_back(kOperandType_InstructionId, _pLeft->uIdentifier);
            Operands.emplace_back(kOperandType_InstructionId, _pRight->uIdentifier);
            return this;
        }
    }

    return nullptr;
}

Instruction* Instruction::Branch(BasicBlock* _pTarget)
{
    CHECK_INSTR;

    if (_pTarget != nullptr)
    {
        _pTarget->m_bSource = false;
        pParent->m_bSink = false;
        pParent->m_pTerminator = this;

        kInstruction = kInstruction_Branch;
        Operands.emplace_back(kOperandType_BasicBlockId, _pTarget->GetIdentifier());

        pParent->m_Successors.push_back(_pTarget);
        _pTarget->m_Predecessors.push_back(pParent);
        return this;
    }

    return nullptr;
}

Instruction* Instruction::BranchCond(const Instruction* _pCondtion, BasicBlock* _pTrueTarget, BasicBlock* _pFalseTarget)
{
    CHECK_INSTR;

    if (_pTrueTarget != nullptr && _pFalseTarget != nullptr)
    {
        _pTrueTarget->m_bSource = false;
        _pFalseTarget->m_bSource = false;
        pParent->m_bSink = false;
        pParent->m_pTerminator = this;

        kInstruction = kInstruction_BranchCond;
        Operands.emplace_back(kOperandType_InstructionId, _pCondtion->uIdentifier);
        Operands.emplace_back(kOperandType_BasicBlockId, _pTrueTarget->GetIdentifier());
        Operands.emplace_back(kOperandType_BasicBlockId, _pFalseTarget->GetIdentifier());

        // TODO check condition type

        pParent->m_Successors.push_back(_pTrueTarget);
        _pTrueTarget->m_Predecessors.push_back(pParent);

        pParent->m_Successors.push_back(_pFalseTarget);
        _pFalseTarget->m_Predecessors.push_back(pParent);
        return this;
    }

    return nullptr;
}
#include "InstructionSetLLVMAMD.h"


std::string InstructionSetLLVMAMD::ResolveTypeName(const Function& _Function, const uint64_t _uTypeId)
{
    return ResolveTypeName(_Function.GetCFG().ResolveType(_Function.GetCFG().GetInstruction(_uTypeId)));
}

std::string InstructionSetLLVMAMD::ResolveTypeName(const TypeInfo& _Type)
{
    std::string sType;
    
    switch (_Type.kType)
    {
    case kType_Void:
        sType = "void";
        break;
    case kType_Bool:
        sType = "i1";
        break;
    case kType_Int:
    case kType_UInt:
        sType = "i" + std::to_string(_Type.uElementBits);
        break;
    case kType_Float:
        if (_Type.uElementBits == 16u)
            sType = "half";
        else if(_Type.uElementBits == 32u)
            sType = "float";
        else if (_Type.uElementBits == 32u)
            sType = "double";
        break;
    case kType_Pointer:
        sType = ResolveTypeName(_Type.SubTypes.front()) + '*';
        break;
    case kType_Array:
        sType = '[' + std::to_string(_Type.uElementCount) + " x " + ResolveTypeName(_Type.SubTypes.front()) + ']';
        break;
    case kType_Struct:
        sType = '{';
        for (auto it = _Type.SubTypes.begin(), end = _Type.SubTypes.end() - 1; it != end; ++it)
        {
            sType += ResolveTypeName(*it) + ',';
        }
        sType += ResolveTypeName(_Type.SubTypes.back()) + '}';
        break;
    default:
        break;
    }

    if (_Type.uElementCount > 1u)
    {
        sType = '<' + std::to_string(_Type.uElementCount) + " x " + sType + '>';
    }

    return sType;
}

bool InstructionSetLLVMAMD::SerializeInstruction(const Function& _Function, const Instruction& _Instruction, std::ostream& _OutStream)
{
    const std::vector<Operand>& Operands = _Instruction.GetOperands();
    const ControlFlowGraph& cfg = _Function.GetCFG();

    switch (_Instruction.GetInstruction())
    {
    case kInstruction_Return:
        _OutStream << "\tret ";
        if (Operands.empty() == false)
        {
            _OutStream << '%' << cfg.GetInstruction(Operands[0].uId)->GetAlias() << std::endl;
        }
        else
        {
            _OutStream << "void" << std::endl;
        }
        break;
    case kInstruction_Branch:
        _OutStream << "\tbr label %" << cfg.GetNode(Operands[0].uId)->GetName() << std::endl;
        break;
    case kInstruction_BranchCond:
        _OutStream << "\tbr " << cfg.GetInstruction(Operands[0].uId)->GetAlias();
        _OutStream << ", label %" << cfg.GetNode(Operands[1].uId)->GetName();
        _OutStream << ", label %" << cfg.GetNode(Operands[2].uId)->GetName() << std::endl;
        break;
    case kInstruction_Equal:
        _OutStream << "\t%" << _Instruction.GetAlias() << " = icmp eq " << ResolveTypeName(_Function, cfg.GetInstruction(Operands[0].uId)->GetResultTypeId());
        _OutStream << '%' << cfg.GetInstruction(Operands[1].uId)->GetAlias() << ", " << std::endl;
        break;
    default:
        break;
    }


    return false;
}

bool InstructionSetLLVMAMD::SerializeListing(const Function& _Function, std::ostream& _OutStream)
{
    _OutStream << "define amdgpu_ps ";
    _OutStream << ResolveTypeName(_Function.GetCFG().ResolveType(_Function.GetReturnType()));
    _OutStream << " @" << _Function.GetName() << '(';

    const std::vector<Instruction*>& Parameters = _Function.GetParameters();

    if (Parameters.empty() == false)
    {
        for (auto it = Parameters.begin(), end = Parameters.end(); it != end; ++it)
        {
            Instruction* pParam = (*it);
            _OutStream << ResolveTypeName(_Function.GetCFG().ResolveType(pParam->GetResultTypeId()));
            if (pParam->Is(kDecoration_Divergent))
            {
                _OutStream << " inreg %";
            }
            else
            {
                _OutStream << " in %";
            }

            _OutStream << pParam->GetAlias();

            if (it + 1 != end)
            {
                _OutStream << ", ";
            }
        }
    }

    _OutStream << ") {" << std::endl;

    for (const BasicBlock& BB : _Function.GetCFG())
    {
        if (BB.IsSource() == false) // entry label
        {
            _OutStream << BB.GetName() << ':' << std::endl;
        }

        for (const Instruction& Instr : BB)
        {
            SerializeInstruction(_Function, Instr, _OutStream);
        }
    }

    _OutStream << '}' << std::endl;

    return true;
}

bool InstructionSetLLVMAMD::SerializeBinary(const Function& _Function, std::ostream& _OutStream)
{
    return false;
}

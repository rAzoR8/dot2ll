#include "ControlFlowGraph.h"

BasicBlock* ControlFlowGraph::AddNode(const uint64_t _uIdentifier, const std::string& _sName)
{
    if (auto it = m_NodeMap.find(_uIdentifier); it != m_NodeMap.end())
    {
        return it->second;
    }

    BasicBlock* pNode = &m_Nodes.emplace_back(BasicBlock(_uIdentifier, this, _sName.empty() ? "BB_" + std::to_string(_uIdentifier) : _sName));
    m_NodeMap[_uIdentifier] = pNode;

    return pNode;
}

BasicBlock* ControlFlowGraph::GetNode(const uint64_t uIdentifier) const
{
    if (auto it = m_NodeMap.find(uIdentifier); it != m_NodeMap.end())
    {
        return it->second;
    }

    return nullptr;
}

TypeInfo ControlFlowGraph::ResolveType(const uint64_t _uTypeId) const
{
    return ResolveType(GetInstruction(_uTypeId));
}

TypeInfo ControlFlowGraph::ResolveType(const Instruction* _pType) const
{
    TypeInfo Info;

    if (_pType != nullptr /*&& _pType->GetInstruction() == kInstruction_Type*/)
    {
        const std::vector<Operand>& Operands = _pType->GetOperands();
        Info.kType = static_cast<EType>(Operands[0].uId);
        Info.Decorations = _pType->GetDecorations();

        switch (Info.kType)
        {
        case kType_Void:
        case kType_Bool:
            break;
        case kType_Int:
        case kType_UInt:
        case kType_Float:
            Info.uElementBits = static_cast<uint32_t>(Operands[1].uId);
            Info.uElementCount = static_cast<uint32_t>(Operands[2].uId);
            break;
        case kType_Pointer:
            Info.SubTypes.push_back(ResolveType(Operands[1].uId));
            break;
        case kType_Array:
            Info.uElementCount = static_cast<uint32_t>(Operands[1].uId);
            Info.SubTypes.push_back(ResolveType(Operands[2].uId));
            break;
        case kType_Struct:
            for (auto it = Operands.begin() + 1, end = Operands.end(); it != end; ++it)
            {
                Info.SubTypes.push_back(ResolveType(it->uId));
            }
        default:
            break;
        }
    }

    return Info;
}

Instruction* ControlFlowGraph::GetInstruction(const uint64_t _uId)
{
    return _uId < m_Instructions.size() ? m_Instructions[_uId] : nullptr;
}

Instruction* ControlFlowGraph::GetInstruction(const uint64_t _uId) const
{
    return _uId < m_Instructions.size() ? m_Instructions[_uId] : nullptr;
}
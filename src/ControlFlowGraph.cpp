#include "ControlFlowGraph.h"

BasicBlock* ControlFlowGraph::AddNode(const std::string& _sName)
{
    return AddNode(hlx::Hash(_sName), _sName);
}

BasicBlock* ControlFlowGraph::AddNode(const uint64_t _uHash, const std::string& _sName)
{
    if (auto it = m_NodeIdentifierMap.find(_uHash); it != m_NodeIdentifierMap.end())
    {
        return &m_Nodes[it->second];
    }

    const size_t uIndex = m_Nodes.size();
    m_NodeIdentifierMap[_uHash] = uIndex;
    return &m_Nodes.emplace_back(uIndex, this, _sName.empty() ? "BB_" + std::to_string(uIndex) : _sName);
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

//Instruction* ControlFlowGraph::GetInstruction(const uint64_t _uId)
//{
//    return _uId < m_Instructions.size() ? m_Instructions[_uId] : nullptr;
//}

Instruction* ControlFlowGraph::GetInstruction(const uint64_t _uId) const
{
    return _uId < m_Instructions.size() ? m_Instructions[_uId] : nullptr;
}
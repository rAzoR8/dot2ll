#include "ControlFlowGraph.h"
#include "Function.h"

ControlFlowGraph::ControlFlowGraph(Function* _pParent, const size_t _uMaxNodes) :
    m_pFunction(_pParent)
{
    m_Nodes.reserve(_uMaxNodes);
};

BasicBlock* ControlFlowGraph::FindNode(const std::string& _sName)
{
    if (auto it = m_NodeIdentifierMap.find(hlx::Hash(_sName)); it != m_NodeIdentifierMap.end())
    {
        return &m_Nodes[it->second];
    }

    return nullptr;
}

const BasicBlock* ControlFlowGraph::FindNode(const std::string& _sName) const
{
    if (auto it = m_NodeIdentifierMap.find(hlx::Hash(_sName)); it != m_NodeIdentifierMap.end())
    {
        return &m_Nodes[it->second];
    }

    return nullptr;
}

BasicBlock* ControlFlowGraph::NewNode(const std::string& _sName)
{
    const InstrId uIndex = static_cast<InstrId>(m_Nodes.size());
    const std::string sName = _sName.empty() ? "BB_" + std::to_string(uIndex) : _sName;
    m_NodeIdentifierMap[hlx::Hash(sName)] = uIndex;

    return &m_Nodes.emplace_back(uIndex, this, sName);
}

TypeInfo ControlFlowGraph::ResolveType(const InstrId _uTypeId) const
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
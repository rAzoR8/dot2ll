#include "Function.h"

Function::Function(const std::string& _sName) :
    m_sName(_sName),
    m_pEntryBlock(m_CFG.AddNode(hlx::Hash(m_sName + "_ENTRYPOINT"), m_sName + "_ENTRYPOINT"))
{
}

Instruction* Function::Type(const TypeInfo& _Type)
{
    const uint64_t uHash = _Type.ComputeHash();

    if (auto it = m_Types.find(uHash); it != m_Types.end())
    {
        return it->second;
    }

    Instruction* pInstr = m_pEntryBlock->AddInstruction();

    std::vector<uint64_t> SubTypes;

    for (const TypeInfo& type : _Type.SubTypes)
    {
        SubTypes.push_back(Type(type)->uIdentifier);
    }

    switch (_Type.kType)
    {
    case kType_Void:
        pInstr->Type(_Type.kType, 0u, 0u);
        break;
    case kType_Bool:
        pInstr->Type(_Type.kType, 1u, 0u);
        break;
    case kType_Int:
    case kType_UInt:
    case kType_Float:
        pInstr->Type(_Type.kType, _Type.uElementBits, _Type.uElementCount);
        break;
    case kType_Pointer:
    case kType_Struct:
        pInstr->Type(_Type.kType, 0u, 0u, SubTypes);
        break;
    case kType_Array:
        pInstr->Type(_Type.kType, 0u, _Type.uElementCount, SubTypes);
        break;
    default:
        break;
    }

    m_Types[uHash] = pInstr;

    return pInstr;
}

Instruction* Function::AddParameter(const Instruction* _pType, const uint64_t _uIndex)
{
    Instruction* pParam = m_pEntryBlock->AddInstruction();
    pParam->FunctionParameter(_pType, _uIndex == InvalidId ? m_Parameters.size() : _uIndex);
    m_Parameters.push_back(pParam);

    return pParam;
}

void Function::Finalize()
{
    // find the source and from the entry block
    BasicBlock* pSource = nullptr;

    for (auto it = m_CFG.begin() + 1, end = m_CFG.end(); it != end; ++it)
    {
        if (it->IsSource() && pSource == nullptr)
        {
            pSource = &(*it);
        }

        if (m_pReturnType == nullptr && it->IsSink() && it->GetTerminator() != nullptr)
        {
            if (it->GetTerminator()->uResultTypeId != InvalidId)
            {
                m_pReturnType = m_Types[it->GetTerminator()->uResultTypeId];
            }
            else
            {
                m_pReturnType = Type(TypeInfo(kType_Void));
            }
        }
    }

    m_pEntryBlock->AddInstruction()->Branch(pSource);
}

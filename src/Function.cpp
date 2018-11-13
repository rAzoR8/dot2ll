#include "Function.h"

Function::Function(const std::string& _sName, const CallingConvention _CallConv) :
    m_sName(_sName),
    m_CFG(this),
    m_CallConv(_CallConv)
{
    m_pConstantTypeBlock = m_CFG.NewNode(m_sName + "_ENTRYPOINT");
    m_pConstantTypeBlock->SetVirtual(true);
}

Instruction* Function::Type(const TypeInfo& _Type)
{
    const uint64_t uHash = _Type.ComputeHash();

    if (auto it = m_Types.find(uHash); it != m_Types.end())
    {
        return it->second;
    }

    std::vector<InstrId> SubTypes;

    for (const TypeInfo& type : _Type.SubTypes)
    {
        SubTypes.push_back(Type(type)->uIdentifier);
    }

    Instruction* pInstr = m_pConstantTypeBlock->AddInstruction();

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

Instruction* Function::AddParameter(const Instruction* _pType, const InstrId _uIndex)
{
    Instruction* pParam = m_pConstantTypeBlock->AddInstruction();
    pParam->FunctionParameter(_pType, _uIndex == InvalidId ? static_cast<InstrId>(m_Parameters.size()) : _uIndex);
    m_Parameters.push_back(pParam);

    return pParam;
}

bool Function::EnforceUniqueExitPoint()
{
    bool bSink = false;

    for (auto it = m_CFG.begin() + 1, end = m_CFG.end(); it != end; ++it)
    {
        if (it->IsSink())
        {
            if (bSink == false)
            {
                bSink = true;
            }
            else
            {
                if (m_pUniqueSink == nullptr)
                {
                    m_pUniqueSink = m_CFG.NewNode(m_sName + "_EXITPOINT");
                    m_pUniqueSink->SetVirtual(true);
                }

                if (it->m_pTerminator != nullptr && it->m_pTerminator->kInstruction == kInstruction_Return)
                {
                    if (m_pReturnType == nullptr)
                    {
                        m_pReturnType = m_Types[it->m_pTerminator->uResultTypeId];
                    }

                    it->m_pTerminator = nullptr; // disable check
                }

                it->AddInstruction()->Branch(m_pUniqueSink);
            }
        }
    }

    if (bSink == false)
    {
        HERROR("No unique exite block found (possible cycle) in function %s", WCSTR(m_sName));    
    }

    return bSink;
}

bool Function::EnforceUniqueEntryPoint()
{
    // no user code
    if (m_CFG.GetNodes().size() < 2)
        return false;

    std::vector<BasicBlock*> Entries;
    for (auto it = m_CFG.begin() + 1, end = m_CFG.end(); it != end; ++it)
    {
        if (it->IsSource())
        {
            Entries.push_back(&(*it));
        }
    }

    // no source node (due to loop at entry)
    if (Entries.empty())
    {
        // promote first user block to source
        (m_CFG.begin() + 1)->SetSource(true);
    }
    else if (Entries.size() > 1u)
    {
        // multiple sources, use the first one, degrade others
        for (auto it = Entries.begin() + 1, end = Entries.end(); it != end; ++it)
        {
            (*it)->SetSource(false);
        }
    }

    return true;
}

// function has no effect if it has already been finalized
void Function::Finalize()
{
    BasicBlock* pSink = GetExitBlock();

    // workaround for now
    if (pSink != nullptr && pSink->GetInstructions().empty())
    {
        pSink->AddInstruction()->Return();
    }

    // todo: generate Phi nodes

    if (m_pReturnType == nullptr)
    {
       m_pReturnType = Type(TypeInfo(kType_Void));
    }

    // connect virtual entry block with user code blocks
    if (m_pConstantTypeBlock->GetTerminator() == nullptr)
    {
        BasicBlock* pSource = GetEntryBlock();
        HASSERT(pSource != nullptr, "No valid/unique entry point found!");
        m_pConstantTypeBlock->AddInstruction()->Branch(pSource);
    }
}

DominatorTree Function::GetDominatorTree() const
{
    return DominatorTree(GetEntryBlock(), false);
}

DominatorTree Function::GetPostDominatorTree() const
{
    return DominatorTree(GetExitBlock(), true);
}

BasicBlock* Function::GetEntryBlock()
{
    return const_cast<BasicBlock*>(const_cast<const Function*>(this)->GetEntryBlock());
}

const BasicBlock* Function::GetEntryBlock() const
{
    if (m_pConstantTypeBlock->GetTerminator() != nullptr)
    {
        return m_pConstantTypeBlock;
    }

    for (auto it = m_CFG.begin() + 1, end = m_CFG.end(); it != end; ++it)
    {
        if (it->IsSource())
        {
            return &(*it);
        }
    }

    if (m_CFG.GetNodes().size() > 1)
    {
        return &*(m_CFG.begin() + 1);
    }

    return nullptr;
}

BasicBlock* Function::GetExitBlock()
{
    return const_cast<BasicBlock*>(const_cast<const Function*>(this)->GetExitBlock());
}

const BasicBlock* Function::GetExitBlock() const
{
    if (m_pUniqueSink != nullptr)
    {
        return m_pUniqueSink;
    }

    for (auto it = m_CFG.rbegin(), end = m_CFG.rend(); it != end; ++it)
    {
        if (it->IsSink())
        {
            return &(*it);
        }
    }

    return nullptr;
}

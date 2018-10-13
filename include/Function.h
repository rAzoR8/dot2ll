#pragma once

#include "ControlFlowGraph.h"
#include "DominatorTree.h"

struct CallingConvention
{
    uint32_t uIdentifier = 0u; // placeholder data for now
};

class Function
{
public:
    Function(const std::string& _sName = "func", const CallingConvention _CallConv = {});

    Function(Function&& _Other) :
        m_sName(std::move(_Other.m_sName)),
        m_CFG(std::move(_Other.m_CFG)),
        //m_pEntryBlock(_Other.m_pEntryBlock),
        m_pConstantTypeBlock(_Other.m_pConstantTypeBlock),
        m_pUniqueSink(_Other.m_pUniqueSink),
        m_Parameters(std::move(_Other.m_Parameters)),
        m_pReturnType(_Other.m_pReturnType),
        m_Types(std::move(_Other.m_Types)),
        m_CallConv(std::move(_Other.m_CallConv))
    {
        for (BasicBlock& BB : m_CFG)
        {
            BB.m_pParent = &m_CFG;
        }
        m_CFG.m_pFunction = this;
    }

    ~Function() {};
    
    Instruction* Type(const TypeInfo& _Type);

    Instruction* AddParameter(const Instruction* _pType, const InstrId _uIndex = InvalidId);

    template <class T>
    Instruction* Type();

    template <class T>
    Instruction* Constant(const T& _Const);

    const ControlFlowGraph& GetCFG() const { return m_CFG; }
    ControlFlowGraph& GetCFG() { return m_CFG; }

    const std::vector<Instruction*>& GetParameters() const { return m_Parameters; }
    const Instruction* GetReturnType() const { return m_pReturnType; };

    // if multiple sinks exist, create a new unique one, rerout sinks to virtual exit
    void EnforceUniqueExitPoint();
    void Finalize(); // connects virtual entry point with CFG

    // only works if finalize has been called before!
    DominatorTree GetDominatorTree() const;
    DominatorTree GetPostDominatorTree() const;

    const std::string& GetName() const { return m_sName; }

    void SetCallingConvention(const CallingConvention& _CallConv) { m_CallConv = _CallConv; }
    const CallingConvention& GetCallingConvention() const { return m_CallConv; }

    BasicBlock* GetEntryBlock();
    const BasicBlock* GetEntryBlock() const;

    BasicBlock* GetExitBlock();
    const BasicBlock* GetExitBlock() const;

private:
    const std::string m_sName;
    ControlFlowGraph m_CFG;
    BasicBlock* m_pConstantTypeBlock = nullptr;
    //BasicBlock* m_pEntryBlock = nullptr; // OEP
    BasicBlock* m_pUniqueSink = nullptr;

    std::vector<Instruction*> m_Parameters;
    Instruction* m_pReturnType = nullptr;

    // type hash -> instruction
    std::unordered_map<uint64_t, Instruction*> m_Types;

    // type hash -> instruction
    std::unordered_map<uint64_t, Instruction*> m_Constants;

    CallingConvention m_CallConv;
};

template<class T>
inline Instruction* Function::Type()
{
    return Type(TypeInfo::From<T>());
}

template<class T>
inline Instruction* Function::Constant(const T& _Const)
{
    const TypeInfo TInfo(TypeInfo::From<T>());
    const uint64_t uHash = hlx::CombineHashes(TInfo.ComputeHash(), hlx::Hash(_Const));

    if (auto it = m_Constants.find(uHash); it != m_Constants.end())
    {
        return it->second;
    }

    const uint8_t* pData = reinterpret_cast<const uint8_t*>(&_Const);

    std::vector<InstrId> ConstData;
    for (size_t i = 0u; i < sizeof(T); ++i) 
    {
        if (i >= ConstData.size() * sizeof(InstrId))
        {
            ConstData.emplace_back();
        }

        reinterpret_cast<uint8_t*>(ConstData.data())[i] = pData[i];
    }

    Instruction* pType = Type(TInfo);
    Instruction* pConst = m_pConstantTypeBlock->AddInstruction()->Constant(pType, ConstData);

    m_Constants[uHash] = pConst;

    return pConst;
}


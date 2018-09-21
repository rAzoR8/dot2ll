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
        m_pEntryBlock(_Other.m_pEntryBlock),
        m_Parameters(std::move(_Other.m_Parameters)),
        m_pReturnType(_Other.m_pReturnType),
        m_Types(std::move(_Other.m_Types)),
        m_CallConv(std::move(_Other.m_CallConv))
    {
        for (BasicBlock& BB : m_CFG)
        {
            BB.m_pParent = &m_CFG;
        }
    }

    ~Function() {};
    
    Instruction* Type(const TypeInfo& _Type);

    Instruction* AddParameter(const Instruction* _pType, const InstrId _uIndex = InvalidId);

    const ControlFlowGraph& GetCFG() const { return m_CFG; }
    ControlFlowGraph& GetCFG() { return m_CFG; }

    const std::vector<Instruction*>& GetParameters() const { return m_Parameters; }
    const Instruction* GetReturnType() const { return m_pReturnType; };

    void Finalize();

    // only works if finalize has been called before!
    DominatorTree GetDominatorTree() const;
    DominatorTree GetPostDominatorTree() const;

    const std::string& GetName() const { return m_sName; }

    void SetCallingConvention(const CallingConvention& _CallConv) { m_CallConv = _CallConv; }
    const CallingConvention& GetCallingConvention() const { return m_CallConv; }

    BasicBlock* GetEntryBlock() const { return m_pEntryBlock; }
    BasicBlock* GetExitBlock() const { return m_pExitBlock; }

private:
    const std::string m_sName;
    ControlFlowGraph m_CFG;
    BasicBlock* m_pEntryBlock = nullptr;
    BasicBlock* m_pExitBlock = nullptr;

    std::vector<Instruction*> m_Parameters;
    Instruction* m_pReturnType = nullptr;

    // type hash -> instruction id
    std::unordered_map<uint64_t, Instruction*> m_Types;

    CallingConvention m_CallConv;
};

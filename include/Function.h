#pragma once

#include "ControlFlowGraph.h"

class Function
{
public:
    Function(const std::string& _sName = "func");

    Function(Function&& _Other) :
        m_sName(std::move(_Other.m_sName)),
        m_CFG(std::move(_Other.m_CFG)),
        m_pEntryBlock(_Other.m_pEntryBlock),
        m_Parameters(std::move(_Other.m_Parameters)),
        m_pReturnType(_Other.m_pReturnType),
        m_Types(std::move(_Other.m_Types))
    {    
    }

    ~Function() {};
    
    Instruction* Type(const TypeInfo& _Type);

    Instruction* AddParameter(const Instruction* _pType, const uint64_t _uIndex = InvalidId);

    const ControlFlowGraph& GetCFG() const { return m_CFG; }
    ControlFlowGraph& GetCFG() { return m_CFG; }

    const std::vector<Instruction*>& GetParameters() const { return m_Parameters; }
    const Instruction* GetReturnType() const { return m_pReturnType; };

    void Finalize();

    const std::string& GetName() const { return m_sName; }

private:
    const std::string m_sName;
    ControlFlowGraph m_CFG;
    BasicBlock* const m_pEntryBlock;
    BasicBlock* m_pExitBlock = nullptr;

    std::vector<Instruction*> m_Parameters;
    Instruction* m_pReturnType = nullptr;

    // type hash -> instruction id
    std::unordered_map<uint64_t, Instruction*> m_Types;
};

#include "BasicBlock.h"
#include "ControlFlowGraph.h"


BasicBlock::BasicBlock(BasicBlock && _Other) :
    m_uIdentifier(std::move(_Other.m_uIdentifier)),
    m_sName(std::move(_Other.m_sName)),
    m_bSource(std::move(_Other.m_bSource)),
    m_bSink(std::move(_Other.m_bSink)),
    m_bDivergent(std::move(_Other.m_bDivergent)),
    m_Instructions(std::move(_Other.m_Instructions)),
    m_Successors(std::move(_Other.m_Successors)),
    m_Predecessors(std::move(_Other.m_Predecessors)),
    m_pParent(std::move(_Other.m_pParent)),
    m_pTerminator(std::move(_Other.m_pTerminator))
{
    // reset to uninitlialzed state
    _Other.m_bSource = true;
    _Other.m_bSink = true;
    _Other.m_bDivergent = false;
    _Other.m_pTerminator = nullptr;
    //_Other.m_pParent = nullptr;
}

Instruction* BasicBlock::AddInstruction()
{
    Instruction* pInstr = &m_Instructions.emplace_back(static_cast<InstrId>(m_pParent->m_Instructions.size()), this);
    m_pParent->m_Instructions.push_back(pInstr);
    return pInstr;
}

Instruction* BasicBlock::AddInstructionFront()
{
    Instruction* pInstr = &m_Instructions.emplace_front(static_cast<InstrId>(m_pParent->m_Instructions.size()), this);
    m_pParent->m_Instructions.push_back(pInstr);
    return pInstr;
}

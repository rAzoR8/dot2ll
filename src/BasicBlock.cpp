#include "BasicBlock.h"
#include "ControlFlowGraph.h"


Instruction* BasicBlock::AddInstruction()
{
    Instruction* pInstr = &m_Instructions.emplace_back(Instruction(m_pParent->m_Instructions.size(), this));
    m_pParent->m_Instructions.push_back(pInstr);
    return pInstr;
}

Instruction* BasicBlock::AddInstructionFront()
{
    Instruction* pInstr = &m_Instructions.emplace_front(Instruction(m_pParent->m_Instructions.size(), this));
    m_pParent->m_Instructions.push_back(pInstr);
    return pInstr;
}

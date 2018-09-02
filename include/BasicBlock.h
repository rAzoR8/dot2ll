#pragma once

#include <vector>
#include <deque>
#include <string>

template <class TInstruction = std::string>
class BasicBlock
{
public:
    using Successors = std::vector<BasicBlock*>;
    using Predecessors = std::vector<BasicBlock*>;
    using Instructions = std::deque<TInstruction>;

    BasicBlock() {}
    BasicBlock(const Instructions& _Instructions, const Successors& _Successors) :
        m_Instructions(_Instructions), m_Successors(_Successors) {}

    ~BasicBlock() {};

    const Instructions& GetInstructions() const { return m_Instructions; }
    Instructions& GetInstructions() { return m_Instructions; }

    const Successors& GetSuccesors() const { return m_Successors; }
    Successors& GetSuccesors() { return m_Successors; }

    const Predecessors& GetPredecessors() const { return m_Predecessors; }
    Predecessors& GetPredecessors() { return m_Predecessors; }

    BasicBlock* AddSuccessor(BasicBlock* _pSuccessor)
    {
        m_Successors.push_back(_pSuccessor);
        _pSuccessor->m_Predecessors.push_back(this);
        return this;
    }

    std::string sName;
    std::string sComment; // Label
    bool bSource; // Entry
    bool bSink; // Exit
    bool bDivergent; // non uniform

private:
    Instructions m_Instructions;
    Successors m_Successors;
    Predecessors m_Predecessors;
};

using DefaultBB = BasicBlock<>;
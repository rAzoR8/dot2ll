#pragma once

#include <vector>
#include <deque>
#include <string>

struct DefaultNodeAttributes
{
};

struct DefaultEdgeAttributes
{
};

template <class TInstruction = std::string, class TNodeAttributes = DefaultNodeAttributes, class TEdgeAttributes = DefaultEdgeAttributes>
class BasicBlock
{
public:
    struct Successor
    {
        Successor(BasicBlock* _pNode = nullptr, const TEdgeAttributes& _Attributes = {}) :
            pNode(_pNode), Attributes(_Attributes) {}

        BasicBlock* pNode;
        TEdgeAttributes Attributes;
    };

    struct NodeAttributes
    {
        std::string sName;
        std::string sComment; // Label
        bool bSource; // Entry
        bool bSink; // Exit
        bool bDivergent; // non uniform

        TNodeAttributes CustomAttributes;
    };

    using Successors = std::vector<Successor>;
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

    const NodeAttributes& GetAttributes() const { return m_Attributes; }
    NodeAttributes& GetAttributes() { return m_Attributes; }

    BasicBlock* AddSuccessor(BasicBlock* _pSuccessor, const TEdgeAttributes& _Attributes = {})
    {
        m_Successors.push_back(Successor(_pSuccessor, _Attributes));
        _pSuccessor->m_Predecessors.push_back(this);
        return this;
    }

private:
    Instructions m_Instructions;
    Successors m_Successors;
    Predecessors m_Predecessors;

    NodeAttributes m_Attributes;
};

using DefaultBB = BasicBlock<>;
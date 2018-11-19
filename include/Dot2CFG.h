#pragma once

#include "DotGraph.h"
#include "Function.h"
#include "InstructionSetLLVMAMD.h"

class Dot2CFG
{
public:
    Dot2CFG() {};
    ~Dot2CFG() {};

    static Function Convert(const DotGraph& _Graph, const std::string& _sUniformAttribKey = "style", const std::string& _sUniformAttribValue = "dotted");
};

inline Function Dot2CFG::Convert(const DotGraph& _Graph, const std::string& _sUniformAttribKey, const std::string& _sUniformAttribValue)
{
    Function func(_Graph.GetName());

    ControlFlowGraph& cfg = func.GetCFG();

    const auto AddNode = [&cfg](const DotNode& node) -> BasicBlock*
    {
        BasicBlock* pBB = cfg.FindNode(node.GetName());
        if (pBB == nullptr)
        {
            pBB = cfg.NewNode(node.GetName());
        }
        return pBB;
    };

    for (const DotNode& node : _Graph)
    {
        BasicBlock* pNode = AddNode(node);

        BasicBlock* pTrueSucc = nullptr;
        BasicBlock* pFalseSucc = nullptr;

        const size_t uSuccessors = node.GetSuccessors().size();
        if (uSuccessors > 0u)
        {
            pNode->SetDivergent(node.GetSuccessors().front().Attributes.HasValue(_sUniformAttribKey, _sUniformAttribValue) == false);
            pTrueSucc = AddNode(*node.GetSuccessors().front().pNode);
        }
        if (uSuccessors == 2u)
        {
            pFalseSucc = AddNode(*node.GetSuccessors()[1].pNode);
        }
        else if(uSuccessors > 2u)
        {
            HERROR("Too many successors for node %s", WCSTR(node.GetName()));
            return {};
        }

        if (pTrueSucc != nullptr && pFalseSucc != nullptr)
        {
            Instruction* pConstNull = func.Constant(0u);
            Instruction* pType = cfg.GetInstruction(pConstNull->GetResultTypeId());

            Instruction* pParam = func.AddParameter(pType);
            pParam->SetAlias("in_" + pNode->GetName());

            Instruction* pCondition = pNode->AddInstruction()->Equal(pParam, pConstNull);
            pCondition->SetAlias("cc_" + pNode->GetName());

            pNode->AddInstruction()->BranchCond(pCondition, pTrueSucc, pFalseSucc);

            if (pNode->IsDivergent())
            {
                pParam->Decorate({ kDecoration_Divergent });
            }
        }
        else if (pTrueSucc != nullptr)
        {
            pNode->AddInstruction()->Branch(pTrueSucc);
        }      
    }

    return func;
}
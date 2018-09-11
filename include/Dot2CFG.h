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
        return cfg.AddNode(hlx::Hash(node.GetName()), node.GetName());
    };

    for (const DotNode& node : _Graph)
    {
        BasicBlock* pNode = AddNode(node);

        BasicBlock* pTrueSucc = nullptr;
        BasicBlock* pFalseSucc = nullptr;

        if (node.GetSuccessors().size() > 0u)
        {
            pNode->SetDivergent(node.GetSuccessors().front().Attributes.HasValue(_sUniformAttribKey, _sUniformAttribValue) == false);
            pTrueSucc = AddNode(*node.GetSuccessors().front().pNode);
        }
        if (node.GetSuccessors().size() > 1u)
        {
            pFalseSucc = AddNode(*node.GetSuccessors()[1].pNode);
        }

        if (pTrueSucc != nullptr && pFalseSucc != nullptr)
        {
            Instruction* pType = func.Type(TypeInfo(kType_Int));

            Instruction* pParam = func.AddParameter(pType);
            pParam->SetAlias("in_" + pNode->GetName());

            if (pNode->IsDivergent())
            {
                pParam->Decorate({ kDecoration_Divergent });
            }

            Instruction* pConstNull = pNode->AddInstruction()->Constant(pType, { 0u });
            Instruction* pCondition = pNode->AddInstruction()->Equal(pParam, pConstNull);
            pCondition->SetAlias("cc_" + pNode->GetName());

            pNode->AddInstruction()->BranchCond(pCondition, pTrueSucc, pFalseSucc);
        }
        else if (pTrueSucc != nullptr)
        {
            pNode->AddInstruction()->Branch(pTrueSucc);
        }      
    }

    func.Finalize();
    return func;
}
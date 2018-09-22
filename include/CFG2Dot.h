#pragma once

#include "ControlFlowGraph.h"
#include "DotGraph.h"

class CFG2Dot
{
public:
    CFG2Dot() {};
    ~CFG2Dot() {};

    static DotGraph Convert(const ControlFlowGraph& _CFG, const std::string& _sName = {}, const std::string& _sUniformAttribKey = "style", const std::string& _sUniformAttribValue = "dotted");
};

DotGraph CFG2Dot::Convert(const ControlFlowGraph& _CFG, const std::string& _sName, const std::string& _sUniformAttribKey, const std::string& _sUniformAttribValue)
{
    DotGraph dot(kGraphType_Directed, _sName);

    for (const BasicBlock& BB : _CFG)
    {
        DotNode* pNode = dot.AddNode(BB.GetName());
        TAttributes EdgeAttribts;
        if (BB.IsDivergent() == false)
        {
            EdgeAttribts[_sUniformAttribKey] = _sUniformAttribValue;
        }

        for (BasicBlock* pSucc : BB.GetSuccesors())
        {
            pNode->AddSuccessor(dot.AddNode(pSucc->GetName()), EdgeAttribts);
        }
    }

    return dot;
}
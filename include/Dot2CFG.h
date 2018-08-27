#pragma once

#include "DotGraph.h"
#include "ControlFlowGraph.h"

class Dot2CFG
{
public:
    Dot2CFG() {};
    ~Dot2CFG() {};

    static DefaultCFG Convert(const DotGraph& _Graph, const std::string& _sDivergenceAttribKey = "style", const std::string& _sDivergenceAttribValue = "dotted");

private:

};

inline DefaultCFG Dot2CFG::Convert(const DotGraph& _Graph, const std::string& _sDivergenceAttribKey, const std::string& _sDivergenceAttribValue)
{
    DefaultCFG cfg;

    const auto AddNode = [&cfg](const DotNode& node) ->DefaultBB*
    {
        const std::hash<std::string> hash;

        DefaultBB* pNode = cfg.AddNode(hash(node.GetName()));
        DefaultNodeAttributes& Attributes = pNode->GetAttributes();

        Attributes.sName = node.GetName();
        Attributes.sComment = node.GetAttributes().GetValue("label");
        Attributes.bSink = node.GetSuccessors().empty();

        return pNode;
    };

    for (const DotNode& node : _Graph)
    {
        DefaultBB* pNode = AddNode(node);

        for (const DotNode::Successor& succ : node.GetSuccessors())
        {
            pNode->GetAttributes().bDivergent = succ.Attributes.HasValue(_sDivergenceAttribKey, _sDivergenceAttribValue);
            pNode->AddSuccessor(AddNode(*succ.pNode), {});
        }
    }

    // multiple sources and sinks might exist with this definition
    for (DefaultBB& BB : cfg)
    {
        BB.GetAttributes().bSource = BB.GetPredecessors().empty();
    }

    return cfg;
}

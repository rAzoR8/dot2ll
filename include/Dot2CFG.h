#pragma once

#include "DotGraph.h"
#include "ControlFlowGraph.h"

class Dot2CFG
{
public:
    Dot2CFG() {};
    ~Dot2CFG() {};

    template <class TInstruction = std::string>
    static TControlFlowGraph<TInstruction> Convert(const DotGraph& _Graph, const std::string& _sDivergenceAttribKey = "style", const std::string& _sDivergenceAttribValue = "dotted");
};

template<class TInstruction>
inline TControlFlowGraph<TInstruction> Dot2CFG::Convert(const DotGraph& _Graph, const std::string& _sDivergenceAttribKey, const std::string& _sDivergenceAttribValue)
{
    using CFG = TControlFlowGraph<TInstruction>;
    using NodeType = typename CFG::NodeType;

    CFG cfg;

    const auto AddNode = [&cfg](const DotNode& node) -> DefaultBB*
    {
        const std::hash<std::string> hash;

        NodeType* pNode = cfg.AddNode(hash(node.GetName()));

        pNode->sName = node.GetName();
        pNode->sComment = node.GetAttributes().GetValue("label");
        pNode->bSink = node.GetSuccessors().empty();

        return pNode;
    };

    for (const DotNode& node : _Graph)
    {
        NodeType* pNode = AddNode(node);

        for (const DotNode::Successor& succ : node.GetSuccessors())
        {
            pNode->bDivergent = succ.Attributes.HasValue(_sDivergenceAttribKey, _sDivergenceAttribValue);
            pNode->AddSuccessor(AddNode(*succ.pNode));
        }
    }

    // multiple sources and sinks might exist with this definition
    for (NodeType& BB : cfg)
    {
        BB.bSource = BB.GetPredecessors().empty();
    }

    return cfg;
}
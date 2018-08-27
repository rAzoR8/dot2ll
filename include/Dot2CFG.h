#pragma once

#include "DotGraph.h"
#include "ControlFlowGraph.h"

class Dot2CFG
{
public:
    Dot2CFG() {};
    ~Dot2CFG() {};

    template <
        class TInstruction = std::string,
        class TNodeAttrib = DefaultNodeAttributes,
        class TEdgeAttrib = DefaultEdgeAttributes>
    static TControlFlowGraph<TInstruction, TNodeAttrib, TEdgeAttrib> Convert(const DotGraph& _Graph, const std::string& _sDivergenceAttribKey = "style", const std::string& _sDivergenceAttribValue = "dotted");

private:

};

template<class TInstruction, class TNodeAttrib, class TEdgeAttrib>
inline TControlFlowGraph<TInstruction, TNodeAttrib, TEdgeAttrib> Dot2CFG::Convert(const DotGraph& _Graph, const std::string & _sDivergenceAttribKey, const std::string & _sDivergenceAttribValue)
{
    using CFG = TControlFlowGraph<TInstruction, TNodeAttrib, TEdgeAttrib>;
    using NodeType = typename CFG::NodeType;

    CFG cfg;

    const auto AddNode = [&cfg](const DotNode& node) -> DefaultBB*
    {
        const std::hash<std::string> hash;

        NodeType* pNode = cfg.AddNode(hash(node.GetName()));
        auto& Attributes = pNode->GetAttributes();

        Attributes.sName = node.GetName();
        Attributes.sComment = node.GetAttributes().GetValue("label");
        Attributes.bSink = node.GetSuccessors().empty();

        return pNode;
    };

    for (const DotNode& node : _Graph)
    {
        NodeType* pNode = AddNode(node);

        for (const DotNode::Successor& succ : node.GetSuccessors())
        {
            pNode->GetAttributes().bDivergent = succ.Attributes.HasValue(_sDivergenceAttribKey, _sDivergenceAttribValue);
            pNode->AddSuccessor(AddNode(*succ.pNode), {});
        }
    }

    // multiple sources and sinks might exist with this definition
    for (NodeType& BB : cfg)
    {
        BB.GetAttributes().bSource = BB.GetPredecessors().empty();
    }

    return cfg;
}
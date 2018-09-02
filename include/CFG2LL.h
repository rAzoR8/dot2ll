#pragma once

#include "Dot2CFG.h"
#include "CFGUtils.h"
#include <ostream>

class CFG2LL
{
public:
    CFG2LL() {};
    ~CFG2LL() {};

    // Adds dummy instruction implementation to the input graph, returns function definition
    static std::string Generate(DefaultCFG& _Graph);

    static void WriteToStdStream(const DefaultCFG& _Graph, const std::string& _sFuncDef, std::ostream& _Stream);

private:

};

inline std::string CFG2LL::Generate(DefaultCFG& _Graph)
{
    std::string sFunctionDef = "define amdgpu_ps void @main(";
    const size_t uFuncDefLen = sFunctionDef.size();

    for (DefaultBB& BB : _Graph)
    {
        auto& Instrutions = BB.GetInstructions();
        const auto& Successors = BB.GetSuccesors();

        Instrutions.push_front(BB.sName + ':');
        switch (Successors.size())
        {
        case 0u:
            Instrutions.push_back("\tret void");
            break;
        case 1u:
            Instrutions.push_back("\tbr label %" + Successors[0]->sName);
            break;
        default: // 2+
        {
            Instrutions.push_back("\t%cc_" + BB.sName + " = icmp eq i32 %in_" + BB.sName + ", 0");
            Instrutions.push_back("\tbr i1 %cc_" + BB.sName + ", label " + Successors[0]->sName + ", label " + Successors[1]->sName);

            // add to func def
            if ((sFunctionDef.size() > uFuncDefLen))
                sFunctionDef += ", ";

            if (BB.bDivergent)
            {
                sFunctionDef += "i32 inreg %in_" + BB.sName;
            }
            else
            {
                sFunctionDef += "i32 in %in_" + BB.sName;
            }
        }
        break;
        }
    }

    sFunctionDef += ")";

    return sFunctionDef;
}

inline void CFG2LL::WriteToStdStream(const DefaultCFG& _Graph, const std::string& _sFuncDef, std::ostream& _Stream)
{
    _Stream << _sFuncDef << '{' << std::endl;

    for (const DefaultBB* pBB : CFGUtils::GetSerializationOrder(_Graph))
    {
        for (const std::string& sInstruction : pBB->GetInstructions())
        {
            _Stream << sInstruction << std::endl;
        }
    }

    _Stream << '}' << std::endl;
}

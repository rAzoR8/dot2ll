#pragma once
#include "Function.h"
#include "DominatorTree.h"

class CheckReconvergence
{
public:

    static bool IsReconverging(const Function& _Func, const bool _bLog = true, const bool _bDisplayAll = false);

};

bool CheckReconvergence::IsReconverging(const Function& _Func, const bool _bLog, const bool _bDisplayAll)
{
    DominatorTree PDT = _Func.GetPostDominatorTree();
    bool bFuncReconv = true;

    for (const BasicBlock& BB : _Func.GetCFG())
    {
        if (BB.IsDivergent())
        {
            const Instruction* pTerminator = BB.GetTerminator();
            HASSERT(pTerminator->Is(kInstruction_BranchCond), "Invalid branch instruction");

            const BasicBlock* pTrueBlock = pTerminator->GetOperandBB(1u);
            const BasicBlock* pFalseBlock = pTerminator->GetOperandBB(2u);

            // one of the successors must post dominate block B and the remaining successor.
            const bool bTrueDomBB = PDT.Dominates(pTrueBlock, &BB);
            const bool bFalseDomBB = PDT.Dominates(pFalseBlock, &BB);

            const bool bTrueDomFalse = /*bTrueDomBB && */ PDT.Dominates(pTrueBlock, pFalseBlock);
            const bool bFalseDomTrue = /*bFalseDomBB && */ PDT.Dominates(pFalseBlock, pTrueBlock);

            const bool bBBReconv = (bTrueDomBB && bTrueDomFalse) || (bFalseDomBB && bFalseDomTrue);

            if (bBBReconv == false)
            {
                if (_bLog)
                {
                    HLOG("%s not Reconverging:", WCSTR(BB.GetName()));
                    HLOG("\t[%s dom %s = %d && %s dom %s = %d]", WCSTR(pTrueBlock->GetName()), WCSTR(BB.GetName()), bTrueDomBB, WCSTR(pTrueBlock->GetName()), WCSTR(pFalseBlock->GetName()), bTrueDomFalse);
                    HLOG("\t[%s dom %s = %d && %s dom %s = %d]", WCSTR(pFalseBlock->GetName()), WCSTR(BB.GetName()), bFalseDomBB, WCSTR(pFalseBlock->GetName()), WCSTR(pTrueBlock->GetName()), bFalseDomTrue);
                }

                bFuncReconv = false;
                if (_bDisplayAll == false)
                {
                    return false;
                }
            }

            //const bool bNot =
            //    (!PDT.Dominates(pTrueBlock, &BB) || !PDT.Dominates(pTrueBlock, pFalseBlock)) &&
            //    (!PDT.Dominates(pFalseBlock, &BB) || !PDT.Dominates(pFalseBlock, pTrueBlock));

            //if (bNot)
            //    return false;
        }
    }
    return bFuncReconv;
}
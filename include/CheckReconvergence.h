#pragma once

#include "DominatorTree.h"

class CheckReconvergence
{
public:

    // for debugging
    static bool IsReconverging(const Function& _Func, const bool _bDisplayAll = false);

    template<class Container>
    static bool IsReconverging(const Container& _BasicBlocks);

    static bool IsReconverging(const BasicBlock* _pBB, const DominatorTree& _PDT);
};

inline bool CheckReconvergence::IsReconverging(const Function& _Func, const bool _bDisplayAll)
{
    DominatorTree PDT = _Func.GetPostDominatorTree();
    bool bFuncReconv = true;

    for (const BasicBlock& BB : _Func.GetCFG())
    {
        if (BB.IsDivergent() /*&& BB.IsVirtual() == false*/)
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
                HLOG("%s not Reconverging:", WCSTR(BB.GetName()));
                HLOG("\t[%s dom %s = %d && %s dom %s = %d]", WCSTR(pTrueBlock->GetName()), WCSTR(BB.GetName()), bTrueDomBB, WCSTR(pTrueBlock->GetName()), WCSTR(pFalseBlock->GetName()), bTrueDomFalse);
                HLOG("\t[%s dom %s = %d && %s dom %s = %d]", WCSTR(pFalseBlock->GetName()), WCSTR(BB.GetName()), bFalseDomBB, WCSTR(pFalseBlock->GetName()), WCSTR(pTrueBlock->GetName()), bFalseDomTrue);
                
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

inline bool CheckReconvergence::IsReconverging(const BasicBlock* _pBB, const DominatorTree& _PDT)
{
    if (_pBB->IsDivergent())
    {
        const Instruction* pTerminator = _pBB->GetTerminator();
        HASSERT(pTerminator->Is(kInstruction_BranchCond), "Invalid branch instruction");

        const BasicBlock* pTrueBlock = pTerminator->GetOperandBB(1u);
        const BasicBlock* pFalseBlock = pTerminator->GetOperandBB(2u);

        if ((!_PDT.Dominates(pTrueBlock, _pBB) || !_PDT.Dominates(pTrueBlock, pFalseBlock)) &&
            (!_PDT.Dominates(pFalseBlock, _pBB) || !_PDT.Dominates(pFalseBlock, pTrueBlock)))
            return false;
    }

    return true;
}

template<class Container>
inline bool CheckReconvergence::IsReconverging(const Container& _BasicBlocks)
{
    BasicBlock* pExit = nullptr;

    for (auto it = _BasicBlocks.begin(), end = _BasicBlocks.end(); it != end; ++it)
    {
        if ((*it)->IsSink())
        {
            pExit = *it;
            break;
        }
    }

    if (pExit == nullptr)
        return false;

    DominatorTree PDT(pExit, true);

    for (const BasicBlock* pBB : _BasicBlocks)
    {
        if (IsReconverging(pBB, PDT) == false)
        {
            return false;
        }
    }

    return true;
}

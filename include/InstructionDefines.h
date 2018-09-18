#pragma once

#include "hlx/include/HashUtils.h"
#include <vector>

#ifdef USE_64BIT_IDS
using InstrId = uint64_t;
#else
using InstrId = uint32_t;
#endif

static constexpr auto InvalidId = std::numeric_limits<InstrId>::max();

enum EOperandType : uint32_t
{
    kOperandType_Constant = 0u,
    kOperandType_InstructionId,
    kOperandType_BasicBlockId // jump target
};

struct Operand
{
    Operand(const EOperandType _kType, const InstrId _uId) :
        kType(_kType), uId(_uId) {}

    EOperandType kType = kOperandType_InstructionId;
    InstrId uId = InvalidId;
};

enum EInstruction : uint32_t
{
    kInstruction_Nop = 0,
    kInstruction_Type,
    kInstruction_FunctionParameter,
    kInstruction_Constant,
    kInstruction_Return,
    kInstruction_Branch,
    kInstruction_BranchCond,
    kInstruction_Equal,
    kInstruction_NotEqual,
    kInstruction_Less,
    kInstruction_LessEqual,
    kInstruction_Greater,
    kInstruction_GreaterEqual,
    kInstruction_Mov,
    kInstruction_Add,
    kInstruction_Sub,
    kInstruction_Mul,
    kInstruction_Div,
    kInstruction_Undefined
};

enum EDecoration : uint32_t
{
    kDecoration_Const = 0u,
    kDecoration_Divergent,
    kDecoration_String,
    kDecoration_UserExt
};

struct Decoration
{
    Decoration(const EDecoration _kDecoration = kDecoration_Const, const uint32_t _uUserData = 0u) :
        kType(_kDecoration), uUserData(_uUserData) {}

    union
    {
        struct
        {
            EDecoration kType;
            uint32_t uUserData;
        };
        uint64_t uData;
    };

    operator uint64_t() const { return uData; }
};

enum EType : uint32_t
{
    kType_Void = 0,
    kType_Bool,
    kType_Int,
    kType_UInt,
    kType_Float,
    kType_Pointer,
    kType_Array,
    kType_Struct,
};

struct TypeInfo
{
    TypeInfo(const EType _kType = kType_Void, const uint32_t _uElementBits = 32u, const uint32_t _uElementCount = 1u) :
        kType(_kType), uElementBits(_uElementBits), uElementCount(_uElementCount) {}

    TypeInfo(const EType _kType, const std::vector<TypeInfo>& _SubTypes) :
        kType(_kType), SubTypes(_SubTypes) {}

    EType kType;
    uint32_t uElementBits;
    uint32_t uElementCount;

    std::vector<TypeInfo> SubTypes;
    std::vector<Decoration> Decorations; // modifiers

    uint64_t ComputeHash() const
    {
        uint64_t uHash = hlx::Hash(kType, uElementBits, uElementCount);

        for (const Decoration& decoration : Decorations)
        {
            uHash = hlx::CombineHashes(uHash, decoration);
        }

        for (const TypeInfo& type : SubTypes)
        {
            uHash = hlx::CombineHashes(uHash, type.ComputeHash());
        }

        return uHash;
    }
};
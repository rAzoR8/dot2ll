#pragma once

#include <ostream>

#include "Function.h"

class InstructionSet
{
public:
    InstructionSet() {};
    ~InstructionSet() {};

    virtual std::string ResolveTypeName(const TypeInfo& _Type) = 0;
    virtual std::string ResolveConstant(const Function& _Function, const Instruction& _Instruction) = 0;
    virtual bool SerializeInstruction(const Function& _Function, const Instruction& _Instruction, std::ostream& _OutStream) = 0;
    virtual bool SerializeListing(const Function& _Function, std::ostream& _OutStream) = 0;
    virtual bool SerializeBinary(const Function& _Function, std::ostream& _OutStream) = 0;
private:

};
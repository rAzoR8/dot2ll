#pragma once

#include "InstructionSet.h"

class InstructionSetLLVMAMD : public InstructionSet
{
public:
    InstructionSetLLVMAMD() {};
    ~InstructionSetLLVMAMD() {};

    std::string ResolveTypeName(const Function& _Function, const uint64_t _uTypeId);

    std::string ResolveTypeName(const TypeInfo& _Type) final;
    bool SerializeInstruction(const Function& _Function, const Instruction& _Instruction, std::ostream& _OutStream) final;
    bool SerializeListing(const Function& _Function, std::ostream& _OutStream) final;
    bool SerializeBinary(const Function& _Function, std::ostream& _OutStream) final;
private:

};

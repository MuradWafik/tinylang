#include "vm/ConstantValue.h"
#include "vm/Chunk.h"

FunctionObject::FunctionObject(std::string name, const size_t num_args, std::unique_ptr<Chunk> chunk)
    : name(std::move(name)), num_args(num_args), chunk(std::move(chunk)) {}

FunctionObject::FunctionObject(std::string name, const size_t num_args, const uint8_t return_bytes, const NativeFn native_fn)
    : name(std::move(name)), num_args(num_args), return_bytes{return_bytes}, native_fn{native_fn} {}

FunctionObject::~FunctionObject() = default;

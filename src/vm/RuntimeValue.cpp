#include "vm/RuntimeValue.h"
#include "vm/Chunk.h"

FunctionObject::FunctionObject(std::string n, const size_t a, std::unique_ptr<Chunk> c)
    : name(std::move(n)), num_args(a), chunk(std::move(c)) {}

FunctionObject::FunctionObject(std::string n, const size_t a, const NativeFn native_fn)
    : name(std::move(n)), num_args(a), native_fn{native_fn} {}

FunctionObject::~FunctionObject() = default;

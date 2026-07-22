#include "vm/RuntimeValue.h"
#include "vm/Chunk.h"

FunctionObject::FunctionObject(std::string n, size_t a, std::unique_ptr<Chunk> c)
    : name(std::move(n)), num_args(a), chunk(std::move(c)) {}

FunctionObject::~FunctionObject() = default;

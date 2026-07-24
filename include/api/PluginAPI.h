#pragma once
#include <vector>
#include "vm/RuntimeValue.h"

// Macro to easily export functions from C++ without mangling their names
// This guarantees the VM can find the function using dlsym
#define TINYLANG_EXPORT extern "C"

// The exact signature every native function must follow
using NativeFn = RuntimeValue (*)(const std::vector<RuntimeValue>&);

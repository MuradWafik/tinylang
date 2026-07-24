#pragma once
#include <vector>
#include "vm/ConstantValue.h"

// Macro to easily export functions from C++ without mangling their names
// This guarantees the VM can find the function using dlsym
#define TINYLANG_EXPORT extern "C"


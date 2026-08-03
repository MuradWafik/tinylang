#include <cstdio>
#include <cstring>
#include <cstdint>
#include <string>
#include "api/PluginAPI.h"
#include "vm/Object.h"

extern "C" {

TINYLANG_EXPORT void Print(const uint8_t* args, uint8_t*)
{
    if(const String* str = *reinterpret_cast<String* const*>(args);
        str
        && str->chars
        && str->length > 0)
    {
        std::fwrite(str->chars, 1, str->length, stdout);
    }
}

TINYLANG_EXPORT void PrintError(const uint8_t* args, uint8_t*)
{
    if(const String* str = *reinterpret_cast<String* const*>(args);
        str
        && str->chars
        && str->length > 0)
    {
        std::fwrite(str->chars, 1, str->length, stderr);
    }
}

TINYLANG_EXPORT void PrintInt(const uint8_t* args, uint8_t*)
{
    const int32_t value = *reinterpret_cast<const int32_t*>(args);
    std::fprintf(stdout, "%d", value);
}

TINYLANG_EXPORT void PrintChar(const uint8_t* args, uint8_t*)
{
    const char value = *reinterpret_cast<const char*>(args);
    fputc(value, stdout);
}

TINYLANG_EXPORT void PrintErrorChar(const uint8_t* args, uint8_t*)
{
    const char value = *reinterpret_cast<const char*>(args);
    fputc(value, stderr);
}

TINYLANG_EXPORT void FlushStdout(const uint8_t*, uint8_t*)
{
    std::fflush(stdout);
}

TINYLANG_EXPORT void IntToString(const uint8_t* args, uint8_t* return_slot)
{
    const int32_t val = *reinterpret_cast<const int32_t*>(args);
    const std::string s = std::to_string(val);
    auto* str_obj = new String(s.c_str(), s.length());
    std::memcpy(return_slot, &str_obj, sizeof(String*));
}

TINYLANG_EXPORT void FloatToString(const uint8_t* args, uint8_t* return_slot)
{
    const float val = *reinterpret_cast<const float*>(args);
    const std::string s = std::to_string(val);
    auto* str_obj = new String(s.c_str(), s.length());
    std::memcpy(return_slot, &str_obj, sizeof(String*));
}

TINYLANG_EXPORT void tinylang_clock(const uint8_t* args, uint8_t* return_slot)
{
    constexpr float time = 42.0f;
    std::memcpy(return_slot, &time, sizeof(std::float32_t));
}

}

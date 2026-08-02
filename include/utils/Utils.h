#pragma once
#include <concepts>
#include <cstring>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <variant>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace FileReader
{
[[nodiscard]] inline std::expected<std::string, std::string> Read(const std::filesystem::path& file_path)
{
    using FailType = std::unexpected<std::string>;
    if(file_path.empty())
    {
        return FailType{"No file provided"};
    }
    if(file_path.extension() != ".tl")
    {
        return FailType{
            std::format("Invalid file type, please provide a .tl file\nFile provided: '{}'", file_path.c_str())
        };
    }
    if(!exists(file_path))
    {
        return FailType{
            std::format("File \"{}\" does not exist", file_path.c_str())
        };
    }

    std::ifstream file(file_path);
    if(!file.is_open())
    {
        return FailType{
            std::format("Unable to open file '{}'", file_path.c_str())
        };
    }

    std::stringstream ss;
    ss << file.rdbuf();

    file.close();
    return ss.str();
}

};

template <std::integral T>
T ReadAndAdvanceBytes(uint8_t*& ip) // apparently needs a REFERENCE to the pointer :))
{
    T result = 0;
    for(size_t i = 0; i < sizeof(T); ++i)
    {
        result = (result << 8) | (*ip++);
    }
    return result;
}

// Fast native-endian write for the VM Stack
template <typename T>
void WriteBytes(std::vector<uint8_t>& stack, const T& value)
{
    const auto bytes = reinterpret_cast<const uint8_t*>(&value);
    stack.insert(stack.end(), bytes, bytes + sizeof(T));
}

// Fast native-endian read and pop from the VM Stack
template <typename T>
T ReadAndPopBytes(std::vector<uint8_t>& stack)
{
    T result;
    const size_t offset = stack.size() - sizeof(T);
    std::memcpy(&result, stack.data() + offset, sizeof(T));
    stack.resize(offset);
    return result;
}

// Fast native-endian peek from the VM Stack (does not pop)
template <typename T>
T ReadBytes(const std::vector<uint8_t>& stack, const size_t offset_from_top = 0)
{
    T result;
    // offset_from_top is the number of bytes from the top of the stack to skip
    const size_t offset = stack.size() - sizeof(T) - offset_from_top;
    std::memcpy(&result, stack.data() + offset, sizeof(T));
    return result;
}

// Fast native-endian read from an absolute byte index in the VM Stack
template <typename T>
T ReadBytesAbsolute(const std::vector<uint8_t>& stack, const size_t absolute_index)
{
    T result;
    std::memcpy(&result, stack.data() + absolute_index, sizeof(T));
    return result;
}

struct StringHash
{
    using is_transparent = void;
    size_t operator()(const std::string_view sv) const
    {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& s) const
    {
        return std::hash<std::string_view>{}(s);
    }
    size_t operator()(const char* s) const
    {
        return std::hash<std::string_view>{}(s);
    }
};


template<typename T>
concept fundamental = std::is_trivially_copyable_v<T>;


template <typename T, typename... Args>
bool IsIn(const T& value, Args&&... args)
{
    using CommonType = std::common_type_t<T, Args...>; // needs a cast when checking in the function

    // stack-allocated list, casting everything to the common base
    std::initializer_list<CommonType> list = { static_cast<CommonType>(value), static_cast<CommonType>(args)... };
    return std::find(list.begin() + 1, list.end(), *list.begin()) != list.end();
}

template <typename T = std::string>
std::optional<T> GetOptional(const nlohmann::json& j, const std::string& key)
{
    if (j.contains(key))
    {
        return j[key].get<T>();
    }
    return std::nullopt;
}


inline bool IsHidden(const std::filesystem::directory_entry& entry)
{
    // Linux / mac
    if(const auto name = entry.path().filename().string(); name.size() > 1 && name[0] == '.')
    {
        return true;
    }

#ifdef _WIN32
    // Windows convention (file attribute check) (ai generated, not tested)
    DWORD attributes = GetFileAttributesW(entry.path().wstring().c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_HIDDEN)) {
        return true;
    }
#endif

    return false;
}


template <typename T>
void WriteValue(std::vector<uint8_t>& buffer, const T& value)
{
    const auto ptr = reinterpret_cast<const uint8_t*>(&value);
    buffer.insert(buffer.end(), ptr, ptr + sizeof(T));
}

template <typename T>
T ReadValue(const std::vector<uint8_t>& buffer, size_t& offset)
{
    T value;
    std::memcpy(&value, buffer.data() + offset, sizeof(T));
    offset += sizeof(T);
    return value;
}

inline void WriteString(std::vector<uint8_t>& buffer, const std::string& str)
{
    WriteValue(buffer, str.size());
    buffer.insert(buffer.end(), str.begin(), str.end());
}

inline std::string ReadString(const std::vector<uint8_t>& buffer, size_t& offset)
{
    const auto size = ReadValue<size_t>(buffer, offset);
    std::string str(buffer.begin() + offset, buffer.begin() + offset + size);
    offset += size;
    return str;
}
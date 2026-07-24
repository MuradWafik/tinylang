#pragma once
#include <concepts>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>


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
    if (!file.is_open())
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
    for (size_t i = 0; i < sizeof(T); ++i)
    {
        result = (result << 8) | (*ip++);
    }
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

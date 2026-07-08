#include "../include/FileReader.h"
#include <format>
#include <fstream>


std::expected<std::string, std::string> FileReader::Read(const std::filesystem::path& file_path)
{
    using FailType = std::unexpected<std::string>;
    if(file_path.empty())
    {
        return FailType{"No file provided"};
    }
    else if(file_path.extension() != ".tl")
    {
        return FailType{
            std::format("Invalid file type, please provide a .tl file\nFile provided: {}", file_path.c_str())
        };
    }
    else if(!exists(file_path))
    {
        return FailType{
            std::format("File \"{}\" does not exist", file_path.c_str())
        };
    }

    std::ifstream file(file_path);
    if (!file.is_open()) {
        return FailType{
            std::format("Unable to open file {}", file_path.c_str())
        };
    }


    std::stringstream ss;
    ss << file.rdbuf();

    file.close();
    return ss.str();
}

#pragma once
#include <expected>
#include <filesystem>


namespace FileReader {
    [[no_discard]] std::expected<std::string, std::string> Read(std::filesystem::path file_path);
};

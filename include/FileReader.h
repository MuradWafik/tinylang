#pragma once
#include <expected>
#include <filesystem>


namespace FileReader {
    [[no_discard]] std::expected<std::string, std::string> Read(const std::filesystem::path& file_path);
};

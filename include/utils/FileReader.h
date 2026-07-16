#pragma once
#include <expected>
#include <filesystem>


namespace FileReader {
    [[nodiscard]] std::expected<std::string, std::string> Read(const std::filesystem::path& file_path);
};

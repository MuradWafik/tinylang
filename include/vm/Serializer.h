#pragma once

#include <expected>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "vm/Chunk.h"

class Serializer
{
public:
    static std::expected<void, std::string> Serialize(const std::unordered_map<std::string, std::unique_ptr<Chunk>>& chunks, const std::vector<std::string>& ordered_modules, const std::string& output_path);
    static std::expected<std::pair<std::unordered_map<std::string, std::unique_ptr<Chunk>>, std::vector<std::string>>, std::string> Deserialize(const std::string& input_path);

private:
    static void SerializeChunk(const Chunk* chunk, std::vector<uint8_t>& buffer);
    static std::unique_ptr<Chunk> DeserializeChunk(const std::vector<uint8_t>& buffer, size_t& offset);
};

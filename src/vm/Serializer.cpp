#include "vm/Serializer.h"

#include <format>
#include <fstream>

#include "utils/Utils.h"


std::expected<void, std::string> Serializer::Serialize(
    const std::unordered_map<std::string, std::unique_ptr<Chunk>>& chunks,
    const std::vector<std::string>& ordered_modules,
    const std::string& output_path)
{
    std::vector<uint8_t> buffer;

    // Header :)
    buffer.push_back('T');
    buffer.push_back('L');
    buffer.push_back('C');
    buffer.push_back('\0');

    WriteValue(buffer, ordered_modules.size());
    for(const auto& mod : ordered_modules)
    {
        WriteString(buffer, mod);
    }

    WriteValue(buffer, chunks.size());
    for(const auto& [name, chunk] : chunks)
    {
        WriteString(buffer, name);
        SerializeChunk(chunk.get(), buffer);
    }

    std::ofstream out(output_path, std::ios::binary);
    if (!out)
    {
        return std::unexpected(std::format("Failed to open file for writing: {}", output_path));
    }

    out.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    return {};
}

// i dont even know a type alias to give this
std::expected
    <std::pair<
        std::unordered_map<std::string, std::unique_ptr<Chunk>>,
        std::vector<std::string>
    >,
    std::string>
Serializer::Deserialize(const std::string& input_path)
{
    std::ifstream in(input_path, std::ios::binary | std::ios::ate);
    if(!in)
    {
        return std::unexpected(std::format("Failed to open file for reading: {}", input_path));
    }

    std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if(!in.read(reinterpret_cast<char*>(buffer.data()), size))
    {
        return std::unexpected("Failed to read file");
    }

    size_t offset = 0;
    if(buffer.size() < 4 || buffer[offset++] != 'T' || buffer[offset++] != 'L' || buffer[offset++] != 'C' || buffer[offset++] != '\0')
    {
        return std::unexpected("Invalid TLC magic number");
    }

    std::vector<std::string> ordered_modules;
    size_t ordered_size = ReadValue<size_t>(buffer, offset);
    for(size_t i = 0; i < ordered_size; ++i)
    {
        ordered_modules.push_back(ReadString(buffer, offset));
    }

    std::unordered_map<std::string, std::unique_ptr<Chunk>> chunks;
    size_t chunks_size = ReadValue<size_t>(buffer, offset);

    for(size_t i = 0; i < chunks_size; ++i)
    {
        std::string name = ReadString(buffer, offset);
        chunks[name] = DeserializeChunk(buffer, offset);
    }

    return std::make_pair(std::move(chunks), std::move(ordered_modules));
}

void Serializer::SerializeChunk(const Chunk* chunk, std::vector<uint8_t>& buffer)
{
    // Write code
    WriteValue(buffer, chunk->code.size());
    buffer.insert(buffer.end(), chunk->code.begin(), chunk->code.end());

    // Write lines
    WriteValue(buffer, chunk->lines.size());
    for (const auto& line : chunk->lines)
    {
        WriteValue(buffer, line);
    }

    // Write functions
    WriteValue(buffer, chunk->functions.size());
    for (const auto& func : chunk->functions)
    {
        WriteString(buffer, func->name);
        WriteValue(buffer, func->num_args);
        SerializeChunk(func->chunk.get(), buffer);
    }

    // Write constants
    WriteValue(buffer, chunk->constants.size());
    for (const auto& constant : chunk->constants)
    {
        WriteValue(buffer, constant.index()); // Store variant index
        
        std::visit([&]<typename T0>(T0&& arg)
        {
            using T = std::decay_t<T0>;
            if constexpr(
                std::is_same_v<T, int32_t>
                || std::is_same_v<T, std::float32_t>
                || std::is_same_v<T, bool>
                || std::is_same_v<T, char8_t>)
            {
                WriteValue(buffer, arg);
            }
            else if constexpr(std::is_same_v<T, std::string>)
            {
                WriteString(buffer, arg);
            }
            else if constexpr(std::is_same_v<T, FunctionObject*>)
            {
                // Find index of FunctionObject in chunk->functions
                size_t func_idx = 0;
                for (size_t i = 0; i < chunk->functions.size(); ++i)
                {
                    if (chunk->functions[i].get() == arg)
                    {
                        func_idx = i;
                        break;
                    }
                }
                WriteValue(buffer, func_idx);
            }
            else if constexpr(std::is_same_v<T, Object*> || std::is_same_v<T, std::monostate>)
            {
                // !!!!! Object* is runtime only, doNT serialize it, not 100% certain on this, but ai says so :))
            }
        }, constant);
    }
    
    WriteValue(buffer, chunk->has_main);
}

std::unique_ptr<Chunk> Serializer::DeserializeChunk(const std::vector<uint8_t>& buffer, size_t& offset)
{
    auto chunk = std::make_unique<Chunk>();

    const auto code_size = ReadValue<size_t>(buffer, offset);
    chunk->code.insert(chunk->code.end(), buffer.begin() + offset, buffer.begin() + offset + code_size);
    offset += code_size;

    const auto lines_size = ReadValue<size_t>(buffer, offset);
    for (size_t i = 0; i < lines_size; ++i)
    {
        chunk->lines.push_back(ReadValue<uint32_t>(buffer, offset));
    }

    const auto funcs_size = ReadValue<size_t>(buffer, offset);
    for (size_t i = 0; i < funcs_size; ++i)
    {
        std::string name = ReadString(buffer, offset);
        size_t num_args = ReadValue<size_t>(buffer, offset);
        auto func_chunk = DeserializeChunk(buffer, offset);
        chunk->functions.push_back(std::make_unique<FunctionObject>(name, num_args, std::move(func_chunk)));
    }

    const auto constants_size = ReadValue<size_t>(buffer, offset);
    for (size_t i = 0; i < constants_size; ++i)
    {
        size_t variant_index = ReadValue<size_t>(buffer, offset);
        switch(variant_index)
        {
            case 0: chunk->constants.push_back(ReadValue<int32_t>(buffer, offset)); break;
            case 1: chunk->constants.push_back(ReadValue<std::float32_t>(buffer, offset)); break;
            case 2: chunk->constants.push_back(ReadValue<bool>(buffer, offset)); break;
            case 3: chunk->constants.push_back(ReadValue<char8_t>(buffer, offset)); break;
            case 4: chunk->constants.push_back(ReadString(buffer, offset)); break;
            case 5:
            {
                const auto func_idx = ReadValue<size_t>(buffer, offset);
                chunk->constants.push_back(chunk->functions[func_idx].get());
                break;
            }
            case 6: // Object* not in serialized file
            case 7: // monostate
                chunk->constants.push_back(std::monostate{});
                break;
            default:
                std::unreachable();
                assert("Unknown variant index" && false);
                // should never happen unless more values are added to the variant
        }
    }
    
    chunk->has_main = ReadValue<bool>(buffer, offset);
    return chunk;
}

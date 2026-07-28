#pragma once
#include <memory>
#include <ranges>
#include <string>
#include <vector>


#include "frontend/Token.h"

#include "utils/Utils.h"

class Type
{
public:
    virtual ~Type() = default;
    // Returns a string representation of the type (e.g., "int", "Point")
    [[nodiscard]] virtual std::string GetName() const = 0;

    // Checks if this type can be assigned to another type
    [[nodiscard]] virtual bool IsAssignableTo(const Type* other) const = 0;
    
    // Returns the size in bytes of the type
    [[nodiscard]] virtual uint8_t GetSize() const = 0;
};

enum class PrimitiveKind
{
    Int,
    Float,
    Bool,
    String,
    Void
};

class PrimitiveType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    [[nodiscard]] PrimitiveKind GetKind() const { return kind; }
    bool IsAssignableTo(const Type* other) const override
    {
        // A primitive is assignable if the other type is the exact same primitive instance
        return this == other;
    }
    [[nodiscard]] bool IsIntegral() const;
    [[nodiscard]] uint8_t GetSize() const override
    {
        switch (kind) {
            case PrimitiveKind::Int:
            case PrimitiveKind::Float:
                return 4;
            case PrimitiveKind::Bool: return 1;
            case PrimitiveKind::String: return 8; // pointer
            case PrimitiveKind::Void: return 0;
        }
        return 0;
    }
    
    PrimitiveType(const PrimitiveKind k, std::string n) : kind(k), name(std::move(n)) {}

    // Singletons for primitives during semantic analysis
    static const std::unique_ptr<PrimitiveType> Int;
    static const std::unique_ptr<PrimitiveType> Float;
    static const std::unique_ptr<PrimitiveType> Bool;
    static const std::unique_ptr<PrimitiveType> String;
    static const std::unique_ptr<PrimitiveType> Void;

private:
    PrimitiveKind kind;
    std::string name;
};

inline const std::unique_ptr<PrimitiveType> PrimitiveType::Int = std::make_unique<PrimitiveType>(PrimitiveKind::Int, "int");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Float = std::make_unique<PrimitiveType>(PrimitiveKind::Float, "float");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Bool = std::make_unique<PrimitiveType>(PrimitiveKind::Bool, "bool");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::String = std::make_unique<PrimitiveType>(PrimitiveKind::String, "String");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Void = std::make_unique<PrimitiveType>(PrimitiveKind::Void, "void");

class FunctionType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] const Type* GetReturnType() const { return return_type; }
    [[nodiscard]] std::vector<const Type*> GetParameters() const { return arguments; }
    bool IsAssignableTo(const Type* other) const override;
    [[nodiscard]] uint8_t GetSize() const override { return 8; } // Function pointer size

    FunctionType(std::vector<const Type*> arguments, const Type* return_type) :
        arguments{std::move(arguments)},
        return_type{return_type}
    { }
private:
    std::vector<const Type*> arguments;
    const Type* return_type{nullptr};
};

class ArrayType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] const Type* GetElementType() const { return element_type; }
    bool IsAssignableTo(const Type* other) const override;
    [[nodiscard]] uint8_t GetSize() const override { return 8; }

    explicit ArrayType(const Type* element_type) :
        element_type{element_type}
    { }
private:
    const Type* element_type{nullptr};
};

class StructType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    [[nodiscard]] const Type* GetFieldType(std::string_view name) const;
    [[nodiscard]] size_t GetNumFields() const { return fields.size(); }
    [[nodiscard]] auto& GetFields() { return fields; }
    [[nodiscard]] const auto& GetFields() const  { return fields; }
    bool IsAssignableTo(const Type* other) const override;
    [[nodiscard]] uint8_t GetSize() const override { return 8; }
    [[nodiscard]] size_t GetHeapSize() const
    {
        size_t total = 0;
        for (const auto& val: fields | std::views::values)
        {
            total += val->GetSize();
        }
        return total;
    }

    StructType(std::string name, std::vector<std::pair<std::string, const Type*>> fields) :
        fields{std::move(fields)}, name{std::move(name)}
    { }

    void SetFields(std::vector<std::pair<std::string, const Type*>> new_fields)
    {
        fields = std::move(new_fields);
    }
private:
    std::vector<std::pair<std::string, const Type*>> fields;
    std::string name;
};

class EnumType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    bool IsAssignableTo(const Type* other) const override;
    [[nodiscard]] uint8_t GetSize() const override { return sizeof(int32_t); }
    std::optional<int32_t> Get(const std::string& name) const
    {
        if(variants.contains(name)) return variants.at(name);
        return std::nullopt;
    }

    EnumType(std::string name, std::unordered_map<std::string, int32_t> variants) :
     name{std::move(name)}, variants{std::move(variants)}
    { }
private:
    std::string name;
    std::unordered_map<std::string, int32_t> variants;
};

class InterfaceType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    [[nodiscard]] auto& GetExpectedMethods() const { return expected_methods; }
    bool IsAssignableTo(const Type* other) const override;
    [[nodiscard]] uint8_t GetSize() const override { return 8; }

    InterfaceType(std::string name, std::vector<std::pair<std::string, const FunctionType*>> expected_methods) :
    name{std::move(name)}, expected_methods{std::move(expected_methods)}
    { }

private:
    std::string name;
    std::vector<std::pair<std::string, const FunctionType*>> expected_methods;
};
#pragma once
#include <algorithm>
#include <memory>
#include <ranges>
#include <string>
#include <vector>


#include "frontend/Token.h"

#include "utils/Mangling.h"
#include "utils/Utils.h"

class InterfaceType;
class FunctionType;

class Type
{
public:
    virtual ~Type() = default;
    // Returns a string representation of the type (e.g., "int", "Point")
    [[nodiscard]] virtual std::string GetName() const = 0;

    // Checks if this type can be assigned to another type
    [[nodiscard]] virtual bool IsAssignableTo(const Type* other) const;
    
    // Returns the size in bytes of the type
    [[nodiscard]] virtual uint8_t GetSize() const = 0;

    [[nodiscard]] virtual uint32_t GetTypeId() const
    {
        return static_cast<uint32_t>(std::hash<std::string>{}(GetName()));
    }

    void AddImplementedInterface(const InterfaceType* interface_type){ implemented_interfaces.push_back(interface_type); }

    bool ImplementsInterface(const std::string& interface_name) const;

    void RegisterMethod(const std::string& method_name, const FunctionType* func_type){ methods[method_name] = func_type; }

    const FunctionType* GetMethod(const std::string& method_name) const
    {
        if(methods.contains(method_name)) return methods.at(method_name);
        return nullptr;
    }

protected:
    std::vector<const InterfaceType*> implemented_interfaces;
    std::unordered_map<std::string, const FunctionType*> methods;
};

enum class PrimitiveKind
{
    Int,
    Float,
    Bool,
    String,
    Char,
    Void,
};

class PrimitiveType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    [[nodiscard]] PrimitiveKind GetKind() const { return kind; }
    [[nodiscard]] bool IsIntegral() const;
    [[nodiscard]] uint8_t GetSize() const override
    {
        switch(kind)
        {
            case PrimitiveKind::Int:
            case PrimitiveKind::Float:
                return 4;
            case PrimitiveKind::Bool:
            case PrimitiveKind::Char:
                return 1;
            case PrimitiveKind::String: return 8; // pointer
            case PrimitiveKind::Void: return 0;
            default: std::unreachable();
        }
    }
    
    PrimitiveType(const PrimitiveKind k, std::string n) : kind(k), name(std::move(n)) {}

    // Singletons for primitives during semantic analysis
    static const std::unique_ptr<PrimitiveType> Int;
    static const std::unique_ptr<PrimitiveType> Float;
    static const std::unique_ptr<PrimitiveType> Bool;
    static const std::unique_ptr<PrimitiveType> String;
    static const std::unique_ptr<PrimitiveType> Char;
    static const std::unique_ptr<PrimitiveType> Void;

private:
    PrimitiveKind kind;
    std::string name;
};

inline const std::unique_ptr<PrimitiveType> PrimitiveType::Int = std::make_unique<PrimitiveType>(PrimitiveKind::Int, "int");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Float = std::make_unique<PrimitiveType>(PrimitiveKind::Float, "float");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Bool = std::make_unique<PrimitiveType>(PrimitiveKind::Bool, "bool");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::String = std::make_unique<PrimitiveType>(PrimitiveKind::String, "String");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Char = std::make_unique<PrimitiveType>(PrimitiveKind::Char, "char");
inline const std::unique_ptr<PrimitiveType> PrimitiveType::Void = std::make_unique<PrimitiveType>(PrimitiveKind::Void, "void");

class FunctionType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] const Type* GetReturnType() const { return return_type; }
    [[nodiscard]] std::vector<const Type*> GetParameters() const { return arguments; }
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


class EnumType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
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
    [[nodiscard]] uint8_t GetSize() const override { return 8; }

    InterfaceType(std::string name, std::vector<std::pair<std::string, const FunctionType*>> expected_methods) :
    name{std::move(name)}, expected_methods{std::move(expected_methods)}
    { }

private:
    std::string name;
    std::vector<std::pair<std::string, const FunctionType*>> expected_methods;
};

class StructType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    [[nodiscard]] const Type* GetFieldType(std::string_view name) const;
    [[nodiscard]] size_t GetNumFields() const { return fields.size(); }
    [[nodiscard]] auto& GetFields() { return fields; }
    [[nodiscard]] const auto& GetFields() const  { return fields; }
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

class ModuleType final : public Type
{
public:
    [[nodiscard]] std::string GetName() const override { return name; }
    [[nodiscard]] uint8_t GetSize() const override { return 0; } // Modules are not values in memory

    explicit ModuleType(std::string name) : name{std::move(name)} {}
    
private:
    std::string name;
};

inline std::string MangleMethodName(const std::string& method_name, const Type* type)
{
    return Mangling::MethodName(type->GetName(), method_name);
}


class AnyType final : public Type
{
public:
    static const std::unique_ptr<AnyType> Instance;

    [[nodiscard]] std::string GetName() const override { return "any"; }
    bool IsAssignableTo(const Type* other) const override { return true; }
    [[nodiscard]] uint8_t GetSize() const override { return 16; } // 8bytes for pointer and 8bytes for type info

    AnyType() = default ;
};

inline const std::unique_ptr<AnyType> AnyType::Instance = std::make_unique<AnyType>();




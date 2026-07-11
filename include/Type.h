#pragma once
#include <memory>
#include <string>
#include <Token.h>
#include <vector>

class Type
{
public:
    virtual ~Type() = default;
    // Returns a string representation of the type (e.g., "int", "Point")
    [[nodiscard]] virtual std::string GetName() const = 0;

    // Checks if this type can be assigned to another type
    [[nodiscard]] virtual bool IsAssignableTo(const Type* other) const = 0;
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

    bool IsAssignableTo(const Type* other) const override {
        // A primitive is assignable if the other type is the exact same primitive instance
        return this == other;
    }
    [[nodiscard]] bool IsIntegral() const;
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


// TODO: Classes for ArrayType and/or StructType depending on roadmap

class FunctionType : public Type
{
public:
    [[nodiscard]] std::string GetName() const override;
    [[nodiscard]] const Type* GetReturnType() const { return return_type; }
    [[nodiscard]] std::vector<const Type*> GetParameters() const { return arguments; }
    bool IsAssignableTo(const Type* other) const override;

    FunctionType(std::vector<const Type*> arguments, const Type* return_type) :
        arguments{std::move(arguments)},
        return_type{return_type}
    { }

private:
    std::vector<const Type*> arguments;
    const Type* return_type{nullptr};
};
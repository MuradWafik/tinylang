#pragma once
#include <cstring>


enum class ObjectType{ Array, String, Struct, Type, Any };

class Object
{
public:
    ObjectType type;
    bool is_marked = false;

    explicit Object(const ObjectType type) : type(type) {}
    virtual ~Object() = default;
};


class String final : public Object
{
public:
    char* chars;
    const size_t length;

    String(const char* chars, const size_t length) :
        Object{ObjectType::String},
        length{length}
    {
        this->chars = new char[length];
        std::memcpy(this->chars, chars, length);
    }

    String(const String& string) = delete;
    String& operator=(const String& string) = delete;

    ~String() override
    {
        delete[] chars;
    }


    // For adding strings to avoid an extra copy
    String(const String* left, const String* right) :
        Object{ObjectType::String},
        length{left->length + right->length}
    {
        this->chars = new char[length];
        std::memcpy(this->chars, left->chars, left->length);
        std::memcpy(this->chars + left->length, right->chars, right->length);
    }
};

class Array final : public Object
{
public:
    size_t size; // Number of elements
    size_t bytes_per_element;
    uint8_t* elements;
    uint8_t element_kind;

    Array(const uint8_t* elements_buffer, const size_t size, const size_t bytes_per_element, const uint8_t element_kind = 0) :
        Object{ObjectType::Array},
        size{size},
        bytes_per_element{bytes_per_element},
        element_kind{element_kind}
    {
        const size_t total_bytes = size * bytes_per_element;
        this->elements = new uint8_t[total_bytes];
        if(elements_buffer && total_bytes > 0)
        {
            std::memcpy(this->elements, elements_buffer, total_bytes);
        }
        else if(total_bytes > 0)
        {
            std::memset(this->elements, 0, total_bytes);
        }
    }

    ~Array() override
    {
        delete[] elements;
    }
};

class Struct final : public Object
{
public:
    size_t size; // Total bytes
    uint8_t* fields;

    Struct(const uint8_t* fields_buffer, const size_t size) :
        Object{ObjectType::Struct},
        size{size}
    {
        this->fields = new uint8_t[size];
        if(fields_buffer && size > 0)
        {
            std::memcpy(this->fields, fields_buffer, size);
        }
        else if(size > 0)
        {
            std::memset(this->fields, 0, size);
        }
    }

    ~Struct() override
    {
        delete[] fields;
    }
};

class TypeObject final : public Object
{
public:
    uint16_t type_id;
    std::string type_name;
    uint8_t size_in_bytes; // primitives have smaller sizes, just based on Type.GetSize()
    TypeObject(const uint16_t id, std::string type_name, const uint8_t size)
        : Object{ObjectType::Type}, type_id(id), type_name(std::move(type_name)), size_in_bytes(size) {}
};


class AnyObject final : public Object
{
public:

    const TypeObject* type_descriptor;
    uint8_t value[8]; // Raw bytes or pointer to another Object*
    AnyObject(const TypeObject* type, const void* data_ptr)
        : Object{ObjectType::Any}, type_descriptor(type)
    {
        std::memcpy(value, data_ptr, type->size_in_bytes);
    }
};
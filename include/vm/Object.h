#pragma once
#include <cstring>


enum class ObjectType{ Array, String, Struct };

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
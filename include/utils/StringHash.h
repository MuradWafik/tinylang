#pragma once
#include <string_view>


struct StringHash {
    using is_transparent = void;
    size_t operator()(const std::string_view sv) const {
        return std::hash<std::string_view>{}(sv);
    }
    size_t operator()(const std::string& s) const {
        return std::hash<std::string>{}(s);
    }
    size_t operator()(const char* s) const {
        return std::hash<std::string_view>{}(s);
    }
};


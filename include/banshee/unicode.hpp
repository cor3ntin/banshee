#pragma once
#include <experimental/text_view>

namespace banshee {
using codepoint = std::experimental::text::unicode_character_set::code_point_type;

inline void push_back(std::string& string, codepoint c) {
    if(c <= 0x7F) {
        string.push_back(c);
        return;
    }
    if(c <= 0x7FF) {
        string.reserve(2);
        string.push_back(0xC0 | (c >> 6));
        string.push_back(0x80 | (c & 0x3F));
        return;
    }
    if(c <= 0xFFFF) {
        string.reserve(3);
        string.push_back(0xE0 | c >> 12);
        string.push_back(0x80 | ((c >> 6) & 0x3F));
        string.push_back(0x80 | (c & 0x3F));
        return;
    }
    if(c <= 0x10FFFF) {
        string.reserve(4);
        string.push_back(0xF0 | (c >> 18));
        string.push_back(0xE0 | ((c >> 12) & 0x3F));
        string.push_back(0x80 | ((c >> 6) & 0x3F));
        string.push_back(0x80 | (c & 0x3F));
        return;
    }
}

inline void push_back(std::u16string& string, codepoint c) {
    if(c < 0x10000) {
        string.push_back(char16_t(c));
        return;
    }
    c -= 0x10000;
    char16_t low = uint16_t((c & 0x3FF) | 0xDC00);
    c >>= 10;
    char16_t high = uint16_t((c & 0x3FF) | 0xD800);
    string.reserve(2);
    string.push_back(high);
    string.push_back(low);
    return;
}

inline void push_back(std::u32string& string, codepoint c) {
    string.push_back(c);
}

inline codepoint surrogate_pair_to_codepoint(char16_t h, char16_t l) {
    return char32_t(h << 10) + l - char32_t(0x35fdc00);
}

}    // namespace banshee

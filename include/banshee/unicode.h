#pragma once
#include <utility>
#include <string>

namespace banshee {

constexpr char32_t end_of_file = 0x03;
constexpr char32_t invalid_character = 0xFFFD;

struct unicode_codepoint {
    char32_t value;
    explicit unicode_codepoint(char32_t value);
    bool valid() const;
    bool eof() const;
    bool is_ascii() const {
        return value < 127;
    }
    operator char() const;
    bool operator==(unicode_codepoint& other) const;
    bool operator==(char& other) const;
};


inline char32_t surrogate_pair_to_codepoint(char16_t h, char16_t l) {
    return char32_t(h << 10) + l - char32_t(0x35fdc00);
}

template<typename Func>
inline bool do_copy_char32_to_utf8_content(const char32_t& c, Func&& push) {
    if(c <= 0x7F) {
        push(c);
        return true;
    }
    if(c <= 0x7FF) {
        push(0xC0 | (c >> 6));
        push(0x80 | (c & 0x3F));
        return true;
    }
    if(c <= 0xFFFF) {
        push(0xE0 | c >> 12);
        push(0x80 | ((c >> 6) & 0x3F));
        push(0x80 | (c & 0x3F));
        return true;
    }
    if(c <= 0x10FFFF) {
        push(0xF0 | (c >> 18));
        push(0xE0 | ((c >> 12) & 0x3F));
        push(0x80 | ((c >> 6) & 0x3F));
        push(0x80 | (c & 0x3F));
        return true;
    }
    return false;
}

inline bool copy_to_string(const unicode_codepoint& c, std::string& out) {
    auto push = [&out](const auto& c) { out.push_back(static_cast<const unsigned char&>(c)); };
    return do_copy_char32_to_utf8_content(c.value, std::move(push));
}

constexpr uint8_t utf8_tail_lengths[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 0, 0};

inline bool copy_to_string(const char* c, std::basic_string<char32_t>& out) {
    int l = utf8_tail_lengths[uint8_t(*c)];
    if(l == 1) {
        out.push_back((uint8_t(*c)));
        return true;
    }
    if(l == 2) {
        out.push_back(char32_t(c[0] & 0x1F) << 6 | (c[1] & 0x3F));
        return true;
    }
    if(l == 3) {
        out.push_back(char32_t((c[0] & 0x0F) << 12) | char32_t((c[1] & 0x3F) << 6) |
                      char32_t(c[2] & 0x3F));
        return true;
    }
    if(l == 4) {
        out.push_back(char32_t((c[0] & 0x07) << 18) | char32_t((c[1] & 0x3F) << 12) |
                      char32_t((c[2] & 0x3F) << 6) | char32_t(c[3] & 0x3F));
        return true;
    }
    return false;
}

/*bool read_next_character(std::basic_istream<char>& s, character<char8_t>& array) {
    bool valid = true;
    s.read(&array.byte<0>(), 1);
    array.byte<1>() = array.byte<2>() = array.byte<3>() = 0;
    if((array.ubyte<0>() & 0xC0) == 0xC0) {
        s.read(&array.byte<1>(), 1);
        valid = valid && (array.byte<1>() & 0x80) == 0x80;
        if((array.ubyte<0>() & 0xE0) == 0xE0) {
            s.read(&array.byte<2>(), 1);
            valid = valid && (array.byte<1>() & 0x80) == 0x80;
            if(array.ubyte<0>() == 0xF0) {
                s.read(&array.byte<3>(), 1);
                valid = valid && (array.byte<1>() & 0x80) == 0x80;
            }
        }
    }
    if(!valid) {
        array.byte<0>() = INVALID_CHARACTER_UTF8[0];
        array.byte<1>() = INVALID_CHARACTER_UTF8[1];
        array.byte<2>() = INVALID_CHARACTER_UTF8[2];
        array.byte<3>() = 0;
    }
    return valid;
}*/


}    // namespace banshee

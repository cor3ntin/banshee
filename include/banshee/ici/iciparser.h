#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include <fstream>
//#include <string_view>
#include <array>
#include "lexer.h"

namespace ici {

namespace details {
    template<typename stream, typename... Args>
    auto make_file(Args... args) {
        return unicode::istream_with_codec<stream>(std::forward<Args...>(args)...);
    }
}    // namespace details

/*
template<class char_type>
class IciParser {
public:
    IciParser(Lexer<char_type>&& lexer) : m_lexer(std::move(lexer)) {}
    bool parse() {
        for(auto token : m_lexer->tokens()) {
            std::cout << token << std::endl;
        }
        return true;
    }

private:
    Lexer<char_type> m_lexer;
};
*/
/*
namespace details {
    template<unicode::codec_name cn, std::size_t bom_size, typename char_type,
             typename String = std::string>
    auto do_make_parser(String&& file_name) {
        auto new_file =
            details::make_file<cn, std::basic_ifstream<unicode::underlying_type_t<cn>>>(file_name);
        new_file.seekg(bom_size);
        auto lexer =
            make_lexer<std::remove_reference_t<decltype(new_file)>, char_type>(std::move(new_file));
        return IciParser<char_type>(std::move(lexer));
    }
}    // namespace details

template<typename char_type>
std::optional<IciParser<char_type>> make_parser(const std::string& file_name) {
    std::ifstream file(file_name, std::ios_base::in);
    if(!file.is_open()) {
        throw std::ios_base::failure("Unable to open " + file_name);
    }

    std::array<char8_t, 4> buff{0, 0, 0, 0};
    constexpr const std::array<char8_t, 3> BOMUTF8{0xEF, 0xBB, 0xBF};
    constexpr const std::array<char8_t, 2> BOMUTF16BE{0xFE, 0xFF};
    constexpr const std::array<char8_t, 2> BOMUTF16LE{0xFF, 0xFE};
    constexpr const std::array<char8_t, 4> BOMUTF32BE{0x00, 0x00, 0xFE, 0xFF};
    constexpr const std::array<char8_t, 4> BOMUTF32LE{0xFF, 0xFE, 0x00, 0x00};


    auto read = file.readsome(reinterpret_cast<char*>(&buff), sizeof(buff));
    if(read < 4) {
        // assume utf 8 - not that it matters anyway
        return details::do_make_parser<unicode::codec_name::utf8, 0, char_type>(file_name);
    }
    auto matches = [file = const_cast<decltype(buff)&>(buff)](auto&& bom) {
        return std::equal(std::begin(bom), std::end(bom), std::begin(file));
    };

    // if(matches(BOMUTF8)) {
    return details::do_make_parser<unicode::codec_name::utf8, 3, char_type>(file_name);
    //}
    if(matches(BOMUTF16LE)) {
        return details::do_make_parser<unicode::codec_name::utf16LE, 2, char_type>(file_name);
    }
    if(matches(BOMUTF16BE)) {
        return details::do_make_parser<unicode::codec_name::utf16BE, 2, char_type>(file_name);
    }
    if(matches(BOMUTF32LE)) {
        return details::do_make_parser<unicode::codec_name::utf32LE, 4, char_type>(file_name);
    }
    if(matches(BOMUTF32BE)) {
        return details::do_make_parser<unicode::codec_name::utf32BE, 4, char_type>(file_name);
    }    throw std::runtime_error("Unexpected codec");
}*/

}    // namespace ici

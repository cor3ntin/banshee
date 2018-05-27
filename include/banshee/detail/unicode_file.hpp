#pragma once
#include <cedilla/normalization.hpp>
#include <boost/endian/conversion.hpp>

namespace banshee::detail {

enum class codec_name { utf8, utf16BE, utf16LE, utf32BE, utf32LE };

template<codec_name name>
constexpr auto codec_endianness = boost::endian::order::native;
template<>
constexpr auto codec_endianness<codec_name::utf16BE> = boost::endian::order::big;
template<>
constexpr auto codec_endianness<codec_name::utf16LE> = boost::endian::order::little;
template<>
constexpr auto codec_endianness<codec_name::utf32BE> = boost::endian::order::big;
template<>
constexpr auto codec_endianness<codec_name::utf32LE> = boost::endian::order::little;

template<codec_name name>
struct bom;

template<>
struct bom<codec_name::utf8> {
    static constexpr const std::array<uint8_t, 3> value{0xEF, 0xBB, 0xBF};
    static constexpr const auto size = value.size();
};

template<>
struct bom<codec_name::utf16BE> {
    static constexpr const std::array<uint8_t, 2> value{0xFE, 0xFF};
    static constexpr const auto size = value.size();
};

template<>
struct bom<codec_name::utf16LE> {
    static constexpr const std::array<uint8_t, 2> value{0xFF, 0xFE};
    static constexpr const auto size = value.size();
};

template<>
struct bom<codec_name::utf32BE> {
    static constexpr const std::array<uint8_t, 4> value{0x00, 0x00, 0xFE, 0xFF};
    static constexpr const auto size = value.size();
};

template<>
struct bom<codec_name::utf32LE> {
    static constexpr const std::array<uint8_t, 4> value{0xFF, 0xFE, 0x00, 0x00};
    static constexpr const auto size = value.size();
};

template<codec_name c>
bool test_bom(const std::array<char, 4>& b) {
    const auto& cn_bom = bom<c>::value;
    return std::equal(std::begin(cn_bom), std::end(cn_bom), std::begin(b));
}


}    // namespace banshee::detail

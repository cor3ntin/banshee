#include "property.h"
#include "catch.hpp"
#include "json_parser.h"
#include <fstream>

bool parse(std::string str) {
    return ici::json_parser<char>(str).parse();
}


TEST_CASE("Json Can be parsed", "[json]") {
    REQUIRE(parse("\"\\u0060\\u012a\\u12AB\""));
    // REQUIRE_FALSE(parse("[`"));
    // REQUIRE(parse("[-0]"));
    // REQUIRE(parse("42"));
    // REQUIRE(parse("-0.1"));
    //  REQUIRE(parse("false"));
    REQUIRE(parse(R"({"a":[{"b": "c"}], "d": "e"})"));
}

#pragma once
#include <banshee/json/json_lexer.h>
#include <banshee/json/json_parser.h>
#include <experimental/filesystem>

namespace banshee {

namespace {
    //       []
}

// CodepointReader, typename CodepointWriter

template<typename PropertyType = property>
json_parser<json_lexer<PropertyType>> create_parser_from_file() {}

}    // namespace banshee

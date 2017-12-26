#include "json_parser.h"
#include <fstream>

int main(int, char** argv) {

    auto parser = ici::json_parser<char>(std::experimental::filesystem::path(std::string(argv[1])));
    return parser.parse() ? 0 : 1;
}

#include <iostream>
#include <banshee/json/json.h>

namespace std {

const char* bad_variant_access::what() const noexcept {
    return "bad_variant_access";
}

}    // namespace std


int main(int, char** argv) {


    auto parser = banshee::create_parser_from_file();

    // auto parser =
    //     ici::json_parser<uint8_t>(std::experimental::filesystem::path(std::string(argv[1])));
    // return parser.parse() ? 0 : 1;


    banshee::property p;
    p = 42;
    assert(p.is_number() && p.is_integral());

    banshee::property o(p);
    std::cout << p.size() << std::endl;
    p = 5.1;

    p = {5, 1};
    assert(p.is_array());
    std::cout << p.size() << p[1] << std::endl;


    // if(!parser)
    //    return 1;
    // parser->parse();
    // return 0;
}

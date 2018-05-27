#include <banshee/banshee.hpp>

int main(int, char** argv) {

    auto view = banshee::json_token_view(banshee::open_unicode_file(argv[1]));
    auto parser = banshee::json_parser(view);
    auto value = parser.parse();
    return value.has_value() ? 0 : 1;
}

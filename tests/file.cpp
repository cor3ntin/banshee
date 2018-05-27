#include <banshee/banshee.hpp>

void dump(banshee::unicode_view& v) {
    for(auto&& c : v) {
        std::cout << char(c);
    }
}

int main() {
    auto f = banshee::open_unicode_file("test.json");


    const std::string s1 = "Hello";
    const std::u16string s2 = u"Hello";
    const std::u32string s3 = U"Hello";

    auto v1 = banshee::unicode_view(s1);
    auto v2 = banshee::unicode_view(s2);
    auto v3 = banshee::unicode_view(s3);

    auto view = banshee::json_token_view(banshee::open_unicode_file("test.json"));
    auto parser = banshee::json_parser(view);
    auto res = parser.parse();

    /*
        for(auto tok :)) {
                std::cout << tok << '\n';
            }
    */

    // for(auto it = l.begin(); it != l.end(); ++it) {
    //}
}

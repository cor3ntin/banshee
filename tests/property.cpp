#include "property.h"
#include "catch.hpp"
#include "utils.h"


TEST_CASE("Properties can be constructed", "[property]") {
    ici::property p_i = 42;
    REQUIRE(p_i.is_number());
    REQUIRE(p_i.is_integral());
    REQUIRE_FALSE(p_i.is_double());
    REQUIRE(p_i == 42);
    REQUIRE(p_i == 42.0);
    REQUIRE(p_i.size() == 0);


    ici::property p_f = std::numeric_limits<double>::max() - 1;
    REQUIRE(p_f.is_number());
    REQUIRE_FALSE(p_f.is_integral());
    REQUIRE(p_f.is_double());
    REQUIRE(p_f == std::numeric_limits<double>::max() - 1);
    REQUIRE_FALSE(p_f == 42.0);
    p_f = 1.0;
    REQUIRE(p_f == 1.0);
    REQUIRE(p_f == 1);
    p_f = 1.5;
    REQUIRE_FALSE(p_f == 1);
    REQUIRE(p_f.size() == 0);

    ici::property p;
    REQUIRE(p.is_null());
    REQUIRE(p.size() == 0);
    REQUIRE(p.is_empty());


    p = p_i;
    REQUIRE(p == 42);
    p = p_f;
    REQUIRE(p == p_f);
    REQUIRE(p != p_i);


    p = "Hello";
    REQUIRE(p == std::string("Hello"));
    REQUIRE(p.is_string());
    REQUIRE(p.size() == std::size_t(std::string("Hello").size()));


    ici::property p_a = {1, 2};
    REQUIRE(p_a.is_array());
    REQUIRE(p_a.size() == 2);

    ici::property p_o = {"1", 2};
    REQUIRE(p_o.is_object());
    REQUIRE(p_o.size() == 1);

    ici::property p_l = {1, 2};
    REQUIRE(p_l.is_array());
    REQUIRE(p_l.size() == 2);

    p_l = {"1", 2, "3"};
    REQUIRE(p_l.is_array());
    REQUIRE(p_l.size() == 3);

    p_l = {2, "1"};
    REQUIRE(p_l.is_array());
    REQUIRE(p_l.size() == 2);


    p_o = {"1", {"1", 2}};
    REQUIRE(p_o.is_object());
    REQUIRE(p_o.size() == 1);
    REQUIRE(p_o["1"].is_object());
    REQUIRE(p_o["1"].size() == 1);

    using property = ici::property;
    using dict = ici::property::dict;
    using list = ici::property::list;

    property obj = dict{"x", list{"foo", "bar"}, "x", 0, "y", 1};

    REQUIRE(obj.is_object());
    REQUIRE(obj["y"] == 1);
    REQUIRE_FALSE(obj["x"].is_array());
    REQUIRE(obj["x"].is_number());
    REQUIRE(obj["x"] == 0);

    property empty_array = property::array_t();
    REQUIRE(empty_array.is_array());
    REQUIRE(empty_array.size() == 0);
}

#pragma once
#include <banshee/json/json_lexer.hpp>
#include <banshee/parser.hpp>
#include <stack>
#include <optional>

namespace banshee {
namespace detail {
    namespace models {
        template<typename T>
        using actual_type = std::remove_cv_t<std::remove_reference_t<std::remove_cv_t<T>>>;
        template<typename TK>
        constexpr bool is_json_token() {
            return std::is_same_v<actual_type<TK>, json_token<typename TK::property_t>>;
        }

        struct JsonTokenInputRange : ranges::concepts::refines<ranges::v3::concepts::InputRange> {
            template<typename C>
            auto
            requires_(C&& c) -> decltype(ranges::concepts::valid_expr(ranges::concepts::is_true(
                std::integral_constant<bool, is_json_token<ranges::range_value_type_t<C>>()>{})));
        };
    }    // namespace models
}    // namespace detail

namespace concepts {
    template<typename T>
    using JsonTokenInputRange = ranges::concepts::models<detail::models::JsonTokenInputRange, T>;
}


template<typename Rng, CONCEPT_REQUIRES_(concepts::JsonTokenInputRange<Rng>())>
class json_parser : public parser_base<Rng> {
    using base = parser_base<Rng>;

public:
    using property_t = typename ranges::range_value_type_t<Rng>::property_t;
    json_parser(Rng& rng) : parser_base<Rng>(rng) {}

    using maybe_property = std::optional<property_t>;
    using TK = typename base::token_t::TokenKind;

    maybe_property do_parse() {

        enum state { parsing_object, parsing_list, parsing_value };
        struct frame {
            state s;
            property p;
            property::key_t k;

            frame(state s) : s(s) {}
            frame(state s, property p) : s(s), p(p) {}

            void set_value(property&& v) {
                if(s == parsing_object) {
                    // assert(!k.empty());
                    p[std::move(k)] = std::move(v);
                } else if(s == parsing_list) {
                    property::array_t(p).push_back(std::move(v));
                } else {
                    p = std::move(v);
                    if(v.is_array()) {
                        s = parsing_list;
                        assert(p.is_empty());
                    } else if(v.is_object()) {
                        assert(p.is_empty());
                        s = parsing_object;
                    }
                }
            }
        };

        std::stack<frame, std::deque<frame>> stack;
        stack.emplace(parsing_value);

    begin:
        auto& token = this->peek_token();
        if(!token) {
            return {};
        }
        auto& top = stack.top();
        switch(stack.top().s) {
            case parsing_object: {
                stack.emplace(parsing_value);
                switch(token) {
                    case TK::tok_rbrace: goto up;
                    case TK::tok_string: {
                        stack.top().k = token.as_string();
                        this->eat_token();
                        auto colon = this->next_token();
                        if(colon != TK::tok_colon)
                            return {};    // expected a colon
                        goto begin;
                    }
                    default:
                        // unexpected token
                        return {};
                }
            }
            case parsing_list: {
                stack.emplace(parsing_value);
                if(token == TK::tok_rsquare)
                    goto up;
                goto begin;
            }
            case parsing_value: {
                switch(token) {
                    case TK::tok_lsquare: {
                        top.set_value(property::array_t{});
                        this->eat_token();
                        goto begin;
                    }
                    case TK::tok_lbrace: {
                        top.set_value(property::object_t{});
                        this->eat_token();
                        goto begin;
                    }
                    case TK::tok_string: top.set_value(token.as_string()); break;
                    case TK::tok_true: top.set_value(true); break;
                    case TK::tok_false: top.set_value(false); break;
                    case TK::tok_integer: top.set_value(token.as_integer()); break;
                    case TK::tok_double: top.set_value(token.as_double()); break;
                    case TK::tok_null: top.set_value(property{}); break;
                    default: { return {}; }
                }
                this->eat_token();
                goto up;
            }
        }
        return {};
    up : {
        auto s = stack.top().s;
        auto nt = this->peek_token();

        if(stack.size() == 1)
            return stack.top().p;

        auto current_property = std::move(stack.top().p);
        stack.pop();
        stack.top().set_value(std::move(current_property));

        s = stack.top().s;
        if((s == parsing_object || s == parsing_list)) {
            switch(nt) {
                case TK::tok_comma: {
                    this->eat_token();
                    nt = this->peek_token();
                    if(nt == TK::tok_rsquare || nt == TK::tok_rbrace) {    // trailing comma
                        return {};
                    }
                    goto begin;
                }
                case TK::tok_rbrace:
                case TK::tok_rsquare:
                    this->eat_token();
                    goto up;    // goto begin, then up
                    // break;
                default:
                    // something that is neither an end of object nor a comma
                    return {};
            }
        }
        goto up;
    }
    }
    bool parse() {
        auto res = do_parse().has_value();
        if(!this->eof() || this->peek_token() != TK::tok_eof)
            return false;
        return res;
    }
};

}    // namespace banshee

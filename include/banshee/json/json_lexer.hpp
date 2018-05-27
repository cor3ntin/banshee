#pragma once
#include <banshee/lexer.hpp>
#include <banshee/property.hpp>
namespace banshee {

namespace detail {

    template<typename property_type = banshee::property>
    struct json_token {
        enum TokenKind {
            tok_eof = 0,
            tok_invalid,
            tok_lbrace,
            tok_rbrace,
            tok_lsquare,
            tok_rsquare,
            tok_string,
            tok_double,
            tok_integer,
            tok_colon,
            tok_comma,
            tok_true,
            tok_false,
            tok_null,
        };
        //#ifndef NDEBUG
        static constexpr const char* token_names[] = {
            "eof",    "invalid", "lbrace", "rbrace", "lsquare", "rsquare", "string",
            "double", "integer", "colon",  "comma",  "true",    "false",   "null"};
        //#endif

        using property_t = property_type;
        using string_t = typename property_type::string_t;
        using integral_t = typename property_type::integral_t;
        using floating_t = typename property_type::floating_t;
        using char_type = typename string_t::value_type;

        TokenKind kind = TokenKind::tok_invalid;
        std::variant<integral_t, floating_t, string_t> value;
        Pos begin, end;
        explicit operator bool() const {
            return kind != TokenKind::tok_eof && kind != TokenKind::tok_invalid;
        }
        operator TokenKind() const {
            return kind;
        }
        auto as_integer() const {
            return std::get<integral_t>(value);
        }

        auto as_double() const {
            return std::get<floating_t>(value);
        }

        const auto& as_string() const {
            return std::get<string_t>(value);
        }
    };
    template<typename property_type>
    std::ostream& operator<<(std::ostream& os, const json_token<property_type>& tok) {
        os << json_token<property_type>::token_names[tok.kind] << " : "
           << " ( " << tok.begin << " ) " << tok.end << " )";
        return os;
    }
    template<typename property_type>
    bool is_eof_token(const json_token<property_type>& t) {
        return t.kind == json_token<property_type>::tok_eof;
    }


}    // namespace detail

template<typename Rng, typename PropertyType = banshee::property,
         CONCEPT_REQUIRES_(cedilla::detail::concepts::CodepointInputRange<Rng>())>
class json_token_view : public lexer_base_view<Rng, json_token_view<Rng, PropertyType>,
                                               detail::json_token<PropertyType>, PropertyType> {

    using base = lexer_base_view<Rng, json_token_view<Rng, PropertyType>,
                                 detail::json_token<PropertyType>, PropertyType>;
    using token_t = typename base::token_t;
    using TokenKind = typename base::TokenKind;
    using Pos = typename base::Pos;


public:
    json_token_view(Rng&& rng) : base(std::forward<Rng>(rng)) {}
    typename base::token_stream_t token_stream() {
        try {
            bool long_string = false;
            while(!this->at_end()) {
                long_string = false;
                typename base::codepoint c = this->getchar();
                switch(c) {
                    case '{': co_yield this->make_token(TokenKind::tok_lbrace); break;
                    case '}': co_yield this->make_token(TokenKind::tok_rbrace); break;
                    case '[': co_yield this->make_token(TokenKind::tok_lsquare); break;
                    case ']': co_yield this->make_token(TokenKind::tok_rsquare); break;
                    case ':': co_yield this->make_token(TokenKind::tok_colon); break;
                    case ',': co_yield this->make_token(TokenKind::tok_comma); break;

                    case '\t':
                    case '\r':
                    case ' ': break;
                    case '\n':
                        this->pos = 0;
                        this->line++;
                        break;
                    case '"': {
                        Pos begin{this->line, this->pos};
                        typename base::string_t str;
                        str.reserve(10);
                        bool escaped = false;
                        char b = c;
                        while(!this->at_end()) {
                            c = this->getchar();
                            if(c == '\\') {
                                escaped = !escaped;
                                continue;
                            }
                            if(escaped) {
                                if(!this->parse_escape_sequence(str, c)) {
                                    co_yield this->make_token(TokenKind::tok_invalid);
                                    break;
                                }
                                escaped = false;
                                continue;
                            } else if(c <= 0x001F) {
                                co_yield this->make_token(TokenKind::tok_invalid);
                                break;
                            }

                            else if(c == b) {
                                Pos end{this->line, this->pos};
                                co_yield this->make_token(TokenKind::tok_string, std::move(str),
                                                          begin, end);
                                break;
                            }
                            banshee::push_back(str, c);
                        }
                        // co_yield make_token(TokenKind::tok_invalid); //uncomplete string
                        break;
                    }
                    case '-':
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        Pos begin{this->line, this->pos};
                        typename base::floating_t d;
                        if(!this->parse_number(d, c)) {
                            co_yield this->make_token(TokenKind::tok_invalid);
                            break;
                        }


                        co_yield this->make_token(TokenKind::tok_double, std::move(d), begin,
                                                  Pos{this->line, this->pos});
                        break;
                    }
                    default: {
                        if(std::isalpha(c) || c == '_' /*|| !c.is_ascii()*/) {
                            Pos begin{base::line, base::pos};
                            typename base::string_t buf;
                            buf.reserve(10);

                            banshee::push_back(buf, c);
                            while(!this->at_end()) {
                                // std::cout << c << std::flush;
                                c = this->peekchar();
                                if(!(isalnum(c) || c == '_' /*|| c.is_ascii()*/))
                                    break;
                                c = this->getchar();
                                banshee::push_back(buf, c);
                            };
                            //  std::cout << buf << std::endl;
                            Pos end{this->line, this->pos};
                            if(buf.compare("false") == 0) {
                                co_yield this->make_token(TokenKind::tok_false, begin, end);
                                break;
                            }
                            if(buf.compare("true") == 0) {
                                co_yield this->make_token(TokenKind::tok_true, begin, end);
                                break;
                            }
                            if(buf.compare("null") == 0) {
                                co_yield this->make_token(TokenKind::tok_null, begin, end);
                                break;
                            }
                            co_yield this->make_token(TokenKind::tok_invalid);
                            break;
                        }
                        co_yield this->make_token(TokenKind::tok_invalid);
                    }
                }    // switch
            }        // while
        } catch(...) {
            co_yield this->make_token(TokenKind::tok_eof);
        }
        co_yield this->make_token(TokenKind::tok_eof);
    }
};


}    // namespace banshee

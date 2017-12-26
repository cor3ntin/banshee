#pragma once
#include <banshee/lexer.h>

namespace banshee {
namespace details {

    class json_lexer_base {

    public:
        enum TokenKind {
            tok_eof,
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
            "double", "colon",   "comma",  "true",   "false",   "null"};
        //#endif
    };

    template<typename property_type>
    struct JsonToken {
        using TokenKind = json_lexer_base::TokenKind;
        using string_t = typename property_type::string_t;
        using integral_t = typename property_type::integral_t;
        using floating_t = typename property_type::floating_t;
        using char_type = typename string_t::value_type;

        json_lexer_base::TokenKind kind = TokenKind::tok_invalid;
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

}    // namespace details

template<typename char_type>
std::enable_if_t<std::is_unsigned_v<char_type>, std::ostream&>
operator<<(std::ostream& os, const std::basic_string<char_type>& s) {
    for(auto&& c : s)
        os << c;
    //<<" ( " << tok.begin << " ) -> ( " << tok.end << " )";
    return os;
}

template<typename property_type>
using json_lexer = basic_lexer<details::JsonToken<property_type>>;

namespace details {

    template<typename CodepointReader, typename CodepointWriter, typename property_type>
    class json_lexer_impl : public json_lexer<property_type>,
                            public lexer_base<CodepointReader, CodepointWriter, property_type> {
    public:
        using Base = lexer_base<CodepointReader, property_type>;
        using Token = JsonToken<property_type>;
        using TokenKind = typename Token::TokenKind;
        using Pos = details::Pos;
        using PropertyType = property_type;

        json_lexer_impl(json_lexer_impl<CodepointReader, CodepointWriter, property_type>& other) =
            delete;
        json_lexer_impl(json_lexer_impl<CodepointReader, CodepointWriter, property_type>&& other) =
            default;
        cppcoro::generator<const Token> token_stream();

    private:
        Token make_token(TokenKind) const;
        template<typename Value>
        Token make_token(TokenKind, Value&& v, Pos begin, Pos end) const;
        Token make_token(TokenKind, Pos begin, Pos end) const;
    };

    template<typename CodepointReader, typename CodepointWriter, typename property_type>
    auto json_lexer_impl<CodepointReader, CodepointWriter, property_type>::token_stream()
        -> cppcoro::generator<const Token> {
        try {
            bool long_string = false;
            while(!this->at_end()) {
                long_string = false;
                unicode_codepoint c = this->getchar();
                switch(c) {
                    case '{': co_yield make_token(TokenKind::tok_lbrace); break;
                    case '}': co_yield make_token(TokenKind::tok_rbrace); break;
                    case '[': co_yield make_token(TokenKind::tok_lsquare); break;
                    case ']': co_yield make_token(TokenKind::tok_rsquare); break;
                    case ':': co_yield make_token(TokenKind::tok_colon); break;
                    case ',': co_yield make_token(TokenKind::tok_comma); break;

                    case '\t':
                    case '\r':
                    case ' ': break;
                    case '\n':
                        this->pos = 0;
                        this->line++;
                        break;
                    case '"': {
                        Pos begin{this->line, this->pos};
                        typename Base::string_type str;
                        str.reserve(10);
                        bool escaped = false;
                        char b = c;
                        while(!this->at_end()) {
                            c = this->getchar();
                            // std::cout << c;
                            if(c == '\\') {
                                escaped = !escaped;
                                continue;
                            }
                            if(escaped) {
                                if(!this->parse_escape_sequence(str, c)) {
                                    co_yield make_token(TokenKind::tok_invalid);
                                    break;
                                }
                                escaped = false;
                                continue;
                            } else if(c <= 0x001F) {
                                co_yield make_token(TokenKind::tok_invalid);
                                break;
                            }

                            else if(c == b) {
                                Pos end{this->line, this->pos};
                                co_yield make_token(TokenKind::tok_string, std::move(str), begin,
                                                    end);
                                break;
                            }
                            m_writer(str, c);
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
                        typename Base::double_type d;
                        if(!this->parse_number(d, c)) {
                            co_yield make_token(TokenKind::tok_invalid);
                            break;
                        }


                        co_yield make_token(TokenKind::tok_double, std::move(d), begin,
                                            Pos{this->line, this->pos});
                        break;
                    }
                    default: {
                        if(std::isalpha(c) || c == '_' || !c.is_ascii()) {
                            Pos begin{Base::line, Base::pos};
                            typename Base::string_type buf;
                            buf.reserve(10);

                            this->m_writer(buf, c);
                            while(!this->at_end()) {
                                std::cout << c << std::flush;
                                c = this->peekchar();
                                if(!(isalnum(c) || c == '_' || c.is_ascii()))
                                    break;
                                c = this->getchar();
                                this->m_writer(buf, c);
                            };
                            std::cout << buf << std::endl;
                            Pos end{this->line, this->pos};
                            if(buf.compare("false") == 0) {
                                co_yield make_token(TokenKind::tok_false, begin, end);
                                break;
                            }
                            if(buf.compare("true") == 0) {
                                co_yield make_token(TokenKind::tok_true, begin, end);
                                break;
                            }
                            if(buf.compare("null") == 0) {
                                co_yield make_token(TokenKind::tok_null, begin, end);
                                break;
                            }
                            co_yield make_token(TokenKind::tok_invalid);
                            break;
                        }
                        co_yield make_token(TokenKind::tok_invalid);
                    }
                }    // switch
            }        // while
        } catch(...) {
            co_yield make_token(TokenKind::tok_eof);
        }
        co_yield make_token(TokenKind::tok_eof);
    }

    template<typename CodepointReader, typename CodepointWriter, typename property_type>
    auto
    json_lexer_impl<CodepointReader, CodepointWriter, property_type>::make_token(TokenKind tk) const
        -> Token {
        return Token{tk, {}, Pos{this->line, this->pos}, Pos{this->line, this->pos}};
    }
    template<typename CodepointReader, typename CodepointWriter, typename property_type>
    template<typename Value>
    auto json_lexer_impl<CodepointReader, CodepointWriter, property_type>::make_token(
        TokenKind tk, Value&& v, Pos begin, Pos end) const -> Token {
        // std::cout << "++" << v << std::endl;
        return Token{tk, std::forward<Value>(v), begin, end};
    }
    template<typename CodepointReader, typename CodepointWriter, typename property_type>
    auto json_lexer_impl<CodepointReader, CodepointWriter, property_type>::make_token(TokenKind tk,
                                                                                      Pos begin,
                                                                                      Pos end) const
        -> Token {
        return Token{tk, {}, begin, end};
    }


    /* template<typename T0, typename... Ts>
     std::ostream& operator<<(std::ostream& s, std::variant<T0, Ts...> const& v) {
         std::visit([&](auto&& arg) { s << arg; }, v);
         return s;
     }*/


    template<typename char_type>
    std::ostream& do_output(std::ostream& os, const typename json_lexer<char_type>::Token& tok) {
        os << json_lexer_base::token_names[tok.kind] << " : "
           << " ( " << tok.begin << " ) " << tok.end << " )";
        return os;
    }

}    // namespace details

/*template<typename char_type>
using JsonLexer = std::unique_ptr<json_lexer<char_type>>;

inline std::ostream& operator<<(std::ostream& os,
                                const typename ici::json_lexer<unsigned char>::Token& tok) {
    return details::do_output<unsigned char>(os, tok);
}

inline std::ostream& operator<<(std::ostream& os,
                                const typename ici::json_lexer<char16_t>::Token& tok) {
    // return details::do_output<char16_t>(os, tok);
}

inline std::ostream& operator<<(std::ostream& os,
                                const typename ici::json_lexer<char32_t>::Token& tok) {
    // return details::do_output<char32_t>(os, tok);
}

template<typename char_type>
struct factory<char_type, json_lexer> {
    template<typename File>
    static auto make_lexer(File&& f) {
        return std::make_unique<details::json_lexer_impl<File, char_type>, File>(
            std::forward<File>(f));
    }
};*/
}    // namespace banshee

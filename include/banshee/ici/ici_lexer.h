#pragma once

/*
 * tatic constexpr std::pair<const char*, details::lexer_base::TokenKind> token_map[] = {
    {"foreach", details::TokenKind::tok_for},
    {"include", details::lexer_base::tok_include},
    {"if", details::lexer_base::tok_if},
    {"else", details::lexer_base::tok_else},
    {"unset", details::lexer_base::tok_unset},
    {"true", details::lexer_base::lexer_base::tok_true},
    {"false", details::lexer_base::lexer_base::tok_false},
    {"null", details::lexer_base::lexer_base::tok_null},
    {"and", details::lexer_base::lexer_base::tok_and},
    {"or", details::lexer_base::lexer_base::tok_or},
    {"local", details::lexer_base::lexer_base::tok_reserved}};

template<typename IStream, typename char_type>
auto LexerImpl<IStream, char_type>::tokens() -> cppcoro::generator<const Token> {
    bool long_string = false;
    while(m_stream.peek() != EOF) {
        long_string = false;
        mbchar_in c = getchar();
        switch(c) {
            case '{': co_yield make_token(TokenKind::tok_lbrace); break;
            case '}': co_yield make_token(TokenKind::tok_rbrace); break;
            case '[': co_yield make_token(TokenKind::tok_lsquare); break;
            case ']': co_yield make_token(TokenKind::tok_rsquare); break;
            case '(': co_yield make_token(TokenKind::tok_lparen); break;
            case ')': co_yield make_token(TokenKind::tok_rparen); break;
            case '=': co_yield make_token(TokenKind::tok_equal); break;
            case ':': co_yield make_token(TokenKind::tok_colon); break;
            case '.': co_yield make_token(TokenKind::tok_dot); break;
            case ',': co_yield make_token(TokenKind::tok_comma); break;
            case '*':
                if(peekchar() == '=') {
                    ;
                    Base::pos++;
                    m_parsed.pop_back();
                    co_yield make_token(TokenKind::tok_star_equal);
                    break;
                }
                break;
                // error;
            case '+': {
                if(peekchar() == '=') {
                    ;
                    Base::pos++;
                    m_parsed.pop_back();
                    co_yield make_token(TokenKind::tok_plus_equal);
                    break;
                }
                break;
                // error
            }

            case '\t':
            case '\f':
            case '\r':
            case ' ': break;
            case '\n':
                Base::pos = 0;
                Base::line++;
                break;
            case '#':    // comments
                while(getchar() != '\n')
                    ;
                Base::line++;
                break;

            case '"':
                if(peekchar(1) == '"' && peekchar(2) == '"') {
                    long_string = true;
                    Base::pos += 2;
                    m_parsed.pop_back();
                    m_parsed.pop_back();
                }
                [[fallthrough]];
            case '\'': {
                Pos begin{Base::line, Base::pos};
                std::basic_string<char_type> str;
                str.reserve(10);
                char8_t dq = 0;
                bool escaped = false;
                mbchar_in b = c;
                while(true) {
                    c = getchar();
                    // std::cout << c;
                    if(c == '\\') {
                        escaped = !escaped;
                        continue;
                    }
                    if(!escaped && c == b) {
                        if(!long_string || ++dq == 3) {
                            Pos end{Base::line, Base::pos};
                            co_yield make_token(Base::tok_string, std::move(str), begin, end);
                            break;
                        }
                    } else if(escaped) {
                        parse_escape_sequence(str, c);
                        escaped = false;
                    } else {
                        dq = 0;
                        decode.copy_to_string(c, str);
                    }
                }
            } break;
            case '-':
                if(peekchar() == '=') {
                    Base::pos++;
                    m_parsed.pop_back();
                    co_yield make_token(Base::tok_minus_equal);
                    break;
                }
                [[fallthrough]];
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                Pos begin{Base::line, Base::pos};
                double d;
                parse_number(d, c);
                co_yield make_token(Base::tok_string, std::move(d), begin,
                                    Pos{Base::line, Base::pos});
                break;
        }    // switch
        if(std::isalpha(c) || c == '_' || c > mbchar_in::max_utf8) {
            Pos begin{Base::line, Base::pos};
            std::basic_string<char_type> buf;
            buf.reserve(10);
            while(true) {
                decode.copy_to_string(c, buf);
                c = peekchar();
                if(!(isalnum(c) || c == '_' || c > mbchar_in::max_utf8))
                    break;
                c = getchar();
            }
            Pos end{Base::line, Base::pos};

            auto it =
                std::find_if(std::begin(token_map), std::end(token_map), [&buf](auto&& v) {
                    return reinterpret_cast<const char8_t*>(v.first) == buf;
                });
            if(it != std::end(token_map)) {
                co_yield make_token(it->second, begin, end);
                continue;
            } else {
                co_yield make_token(Base::tok_id, begin, end);
                continue;
            }
        }
    }    // while
    co_yield make_token(Base::tok_eof);
}
*/


public:
enum TokenKind {
    tok_id,
    tok_eof,
    tok_lparen,
    tok_rparen,
    tok_lbrace,
    tok_rbrace,
    tok_lsquare,
    tok_rsquare,
    tok_string,
    tok_double,
    tok_colon,
    tok_dot,
    tok_comma,
    tok_equal,
    tok_star_equal,
    tok_minus_equal,
    tok_plus_equal,
    tok_for,
    tok_if,
    tok_else,
    tok_unset,
    tok_concat,
    tok_reserved,
    tok_true,
    tok_false,
    tok_null,
    tok_and,
    tok_or,
    tok_include,
};
#ifndef NDEBUG
static constexpr const char* token_names[] = {
    "id",         "eof",    "lparen", "rparen", "lbrace", "rbrace", "lsquare",    "rsquare",
    "string",     "double", "colon",  "dot",    "comma",  "equal",  "star_equal", "minus_equal",
    "plus_equal", "for",    "if",     "else",   "unset",  "concat", "reserved",   "true",
    "false",      "null",   "and",    "or",     "include"};
#endif
}
;

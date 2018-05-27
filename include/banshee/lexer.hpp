#pragma once
#include <optional>
#include <variant>
#include <string_view>
#include <vector>
#include <map>
#include <utility>
#include <iostream>
#include <range/v3/view_facade.hpp>
#include <cedilla/detail/unicode_base_view.hpp>
#include <banshee/detail/generator.hpp>
#include <banshee/unicode.hpp>


namespace banshee {

namespace detail {
    struct Pos {
        std::size_t line, pos;
    };
    inline std::ostream& operator<<(std::ostream& os, const Pos& pos) {
        os << pos.line << ":" << pos.pos;
        return os;
    }


    struct basic_types {
        using bool_t = bool;
        using char_t = char;
        using floating_t = double;
        using integer_t = int64_t;
        using string_t = std::basic_string<char_t>;
    };
}    // namespace detail


template<typename Rng, typename Derived, typename Token, typename Types = detail::basic_types>
class lexer_base_view : public ranges::v3::view_facade<Derived, ranges::finite> {
public:
    using token_t = Token;
    using token_stream_t = cppcoro::generator<const token_t>;
    using string_t = typename Types::string_t;
    using floating_t = typename Types::floating_t;
    using integral_t = typename Types::integral_t;
    using codepoint = typename ranges::v3::value_type_t<Rng>;

protected:
    Rng m_rng;

    using iterator_t = decltype(std::begin(m_rng));
    using sentinel_t = decltype(std::end(m_rng));

    token_stream_t m_stream;
    iterator_t m_it;
    sentinel_t m_end;
    std::size_t line = 0;
    std::size_t pos = 0;
    std::vector<codepoint> m_parsed;

    bool at_end() {
        return m_it == m_end && m_parsed.empty();
    }
    codepoint getchar() {
        codepoint c;
        if(!m_parsed.empty()) {
            c = m_parsed.back();
            m_parsed.pop_back();
        } else {
            c = *m_it;
            ++m_it;
        }
        line++;
        return c;
    }
    codepoint peekchar(std::size_t n = 1) {
        while(m_parsed.size() < n) {
            m_parsed.insert(m_parsed.begin(), *m_it);
            m_it++;
        }
        return m_parsed[m_parsed.size() - n];
    }


    bool parse_escape_sequence(string_t& out, const codepoint& starting_with);
    bool parse_number(double_t& d, const codepoint& starting_with);
    using Pos = detail::Pos;
    using TokenKind = typename token_t::TokenKind;
    token_t make_token(TokenKind tk) const;
    template<typename Value>
    token_t make_token(TokenKind, Value&& v, Pos begin, Pos end) const;
    token_t make_token(TokenKind, Pos begin, Pos end) const;

public:
    lexer_base_view(Rng&& rng) :
        m_rng(std::forward<Rng>(rng)),
        m_it(std::begin(m_rng)),
        m_end(std::end(m_rng)) {
        m_stream = static_cast<Derived*>(this)->token_stream();
    }

    struct cursor {
        token_stream_t* m_stream = nullptr;

    public:
        cursor(token_stream_t* stream = nullptr) : m_stream(stream) {
            next();
        }
        cursor(const cursor&) = default;

        bool equal(ranges::v3::default_sentinel) const {
            return !m_stream || !m_stream->has_next();
        }

        token_t read() const {
            return m_token;
        }

        void next() {
            m_token = m_stream->next();
        }

    protected:
        token_t m_token;
    };

    cursor begin_cursor() {
        return cursor(&m_stream);
    }
};

template<typename Rng, typename Derived, typename Token, typename Types>
bool lexer_base_view<Rng, Derived, Token, Types>::parse_number(double_t& d,
                                                               const codepoint& starting_with) {
    std::string buffer;
    buffer.reserve(5);
    char c = starting_with;
    bool leading = true;
    bool previous_isDigit = isdigit(c);

    while(true) {
        buffer.push_back(c);
        if(this->at_end())
            break;
        c = peekchar();
        const bool isDigit = isdigit(c);
        const bool isExp = c == 'E' || c == 'e';

        if(!(isDigit || isExp || c == '.' || c == '-' || c == '+'))
            break;

        const char previous = buffer[buffer.size() - 1];

        if(leading && previous == '0' && isDigit)
            return false;

        if(isExp && previous == '.')
            return false;

        if(c == '.' && !previous_isDigit)
            return false;

        if(c != '0')
            leading = false;
        previous_isDigit = isDigit;

        c = this->getchar();
    }
    if(buffer.size() > 0 && buffer[buffer.size() - 1] == '.')
        return false;
    if(buffer.size() == 1 && buffer[0] == '-')
        return false;

    char* end;
    d = std::strtold(buffer.c_str(), &end);    // may throw
    return end == buffer.c_str() + buffer.size();
}


template<typename Rng, typename Derived, typename Token, typename Types>
bool lexer_base_view<Rng, Derived, Token, Types>::parse_escape_sequence(
    string_t& out, const codepoint& starting_with) {
    switch(starting_with) {
        case 'b': banshee::push_back(out, '\b'); return true;
        case 'f': banshee::push_back(out, '\f'); return true;
        case 'n': banshee::push_back(out, '\n'); return true;
        case 'r': banshee::push_back(out, '\r'); return true;
        case 't': banshee::push_back(out, '\t'); return true;
        case 'v': banshee::push_back(out, '\v'); return true;
        case '"': banshee::push_back(out, '"'); return true;
        case '/': banshee::push_back(out, '/'); return true;
        case '\'': banshee::push_back(out, '\''); return true;
        case '\\': banshee::push_back(out, '\\'); return true;
        case '0': {
            if(!std::isdigit(peekchar())) {
                banshee::push_back(out, '\\');
                return true;
            }
            // Octal
            return false;
        }
        // Hexa
        case 'x': {
            return false;
        }
        // unicode
        case 'u': {
            std::string unicode(4, 0);
            try {
                size_t res = 0;
                for(char i = 0; i < 4; i++)
                    unicode[i] = this->getchar();
                char32_t codepoint = std::stoul(unicode, &res, 16);
                if(res != 4)
                    return false;
                if(peekchar(1) == '\\' && peekchar(2) == 'u') {
                    this->getchar();
                    this->getchar();
                    for(char i = 0; i < 4; i++)
                        unicode[i] = this->getchar();

                    auto low = std::stoul(unicode, &res, 16);
                    if(res != 4)
                        return false;
                    codepoint = surrogate_pair_to_codepoint(codepoint, low);
                }
                banshee::push_back(out, codepoint);
            } catch(std::exception e) {
                // std::cout << e.what() << unicode << std::endl;
                return false;
            }
            return true;
        }

        default: return false;
    }
    assert(false);
    return false;
}

template<typename Rng, typename Derived, typename Token, typename Types>
auto lexer_base_view<Rng, Derived, Token, Types>::make_token(TokenKind tk) const -> token_t {
    return token_t{tk, {}, Pos{this->line, this->pos}, Pos{this->line, this->pos}};
}

template<typename Rng, typename Derived, typename Token, typename Types>
template<typename Value>
auto lexer_base_view<Rng, Derived, Token, Types>::make_token(TokenKind tk, Value&& v, Pos begin,
                                                             Pos end) const -> token_t {
    return token_t{tk, std::forward<Value>(v), begin, end};
}

template<typename Rng, typename Derived, typename Token, typename Types>
auto lexer_base_view<Rng, Derived, Token, Types>::make_token(TokenKind tk, Pos begin, Pos end) const
    -> token_t {
    return Token{tk, {}, begin, end};
}


}    // namespace banshee

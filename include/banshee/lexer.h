#pragma once
#include <optional>
#include <variant>
#include <string_view>
#include <vector>
#include <map>
#include <utility>
#include <iostream>

#include <banshee/unicode.h>
#include <banshee/coro.h>


namespace banshee {

struct sized_codepoint : unicode_codepoint {
    uint8_t size;
};

namespace details {
    struct Pos {
        std::size_t line, pos;
    };

    struct basic_types {
        using char_type = char;
        using double_type = double;
        using integer_type = int64_t;
        using string_type = std::basic_string<char_type>;
    };

    template<typename CodepointReader, typename CodepointWriter, typename Types = basic_types>
    class lexer_base {
    public:
        lexer_base(lexer_base<CodepointReader, Types>&& other) = default;

        using char_type = typename Types::char_type;
        using string_type = typename Types::string_type;
        using double_type = typename Types::double_type;
        using integer_type = typename Types::integer_type;


    protected:
        CodepointReader m_reader;
        CodepointWriter m_writer;
        std::size_t line = 0;
        std::size_t pos = 0;

        bool at_end();
        unicode_codepoint getchar();
        unicode_codepoint peekchar(std::size_t n = 1);
        sized_codepoint do_getchar();

        std::vector<unicode_codepoint> m_parsed;

        bool parse_escape_sequence(string_type& out, const sized_codepoint& starting_with);
        bool parse_number(double_type& d, const sized_codepoint& starting_with);
    };

    template<typename CodepointReader, typename CodepointWriter, typename Types>
    bool lexer_base<CodepointReader, CodepointWriter, Types>::parse_escape_sequence(
        string_type& out, const sized_codepoint& starting_with) {
        switch(starting_with) {
            case 'b': m_writer(out, '\b'); return true;
            case 'f': m_writer(out, '\f'); return true;
            case 'n': m_writer(out, '\n'); return true;
            case 'r': m_writer(out, '\r'); return true;
            case 't': m_writer(out, '\t'); return true;
            case 'v': m_writer(out, '\v'); return true;
            case '"': m_writer(out, '"'); return true;
            case '/': m_writer(out, '/'); return true;
            case '\'': m_writer(out, '\''); return true;
            case '\\': m_writer(out, '\\'); return true;
            case '0': {
                if(!std::isdigit(peekchar())) {
                    m_writer(out, '\\');
                    return true;
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
                    m_writer(out, codepoint);
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

    template<typename CodepointReader, typename CodepointWriter, typename Types>
    bool lexer_base<CodepointReader, CodepointWriter, Types>::parse_number(
        double_type& d, const sized_codepoint& starting_with) {
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

    template<typename CodepointReader, typename CodepointWriter, typename Types>
    auto lexer_base<CodepointReader, CodepointWriter, Types>::peekchar(std::size_t n)
        -> unicode_codepoint {
        while(m_parsed.size() < n)
            m_parsed.insert(m_parsed.begin(), do_getchar());
        return m_parsed[m_parsed.size() - n];
    }

    template<typename CodepointReader, typename CodepointWriter, typename Types>
    auto lexer_base<CodepointReader, CodepointWriter, Types>::getchar() -> unicode_codepoint {
        auto get_cached = [this]() {
            auto v = m_parsed.back();
            m_parsed.pop_back();
            return v;
        };
        sized_codepoint c = m_parsed.empty() ? do_getchar() : get_cached();
        pos += c.size;
        return c;
    }

    template<typename CodepointReader, typename CodepointWriter, typename Types>
    auto lexer_base<CodepointReader, CodepointWriter, Types>::do_getchar() -> sized_codepoint {
        const auto cp = m_reader();
        if(cp == end_of_file)
            throw std::runtime_error("EOF");
        return cp;
    }

    template<typename CodepointReader, typename CodepointWriter, typename Types>
    bool lexer_base<CodepointReader, CodepointWriter, Types>::at_end() {
        if(!m_parsed.empty())
            return m_parsed.back() == end_of_file;
        const auto cp = m_reader();
        if(cp == end_of_file)
            return true;
        m_parsed.insert(m_parsed.begin(), cp);
        return false;
    }

}    // namespace details


template<typename TK>
class basic_lexer {
public:
    using Token = TK;

    basic_lexer(basic_lexer<Token>&& other) = default;
    basic_lexer() = default;

    virtual cppcoro::generator<const Token> token_stream() = 0;
    virtual ~basic_lexer() = default;
};


/*inline std::ostream& operator<<(std::ostream& os, const ici::details::Pos& pos) {
    os << pos.line << ":" << pos.pos;
    return os;
}*/

}    // namespace banshee

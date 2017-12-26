#pragma once
#include <banshee/lexer.h>

namespace banshee {

template<typename Lexer>
class parser_base {
public:
    using lexer_type = std::unique_ptr<Lexer>;


private:
    lexer_type lexer;


protected:
    using Token = typename Lexer::Token;
    using TK = typename Token::TokenKind;

    std::vector<Token> m_peeked;
    decltype(lexer->token_stream()) stream{};

    Token next_token() {
        if(!m_peeked.empty()) {
            Token t = m_peeked.back();
            m_peeked.pop_back();
            return t;
        }
        auto token = std::move(stream.next());
        return token;
    }
    const Token& peek_token() {
        if(m_peeked.empty()) {
            m_peeked.push_back(stream.next());
        }
        return m_peeked.back();
    }

    void eat_token() {
        (void)next_token();
    }

    bool eof() {
        if(!m_peeked.empty())
            return peek_token() == Token::TokenKind::tok_eof;
        return (!stream.has_next() || stream.next() == Token::TokenKind::tok_eof);
    }
};

}    // namespace banshee

#pragma once
#include <range/v3/range_concepts.hpp>

namespace banshee {

template<typename Rng, CONCEPT_REQUIRES_(ranges::InputRange<Rng>())>
class parser_base {
public:
    using token_t = typename ranges::range_value_type_t<Rng>;
    parser_base(Rng& rng) : m_rng(rng), m_it(std::begin(m_rng)), m_end(std::end(m_rng)) {}

protected:
    std::vector<token_t> m_peeked;
    token_t next_token() {
        if(!m_peeked.empty()) {
            token_t t = m_peeked.back();
            m_peeked.pop_back();
            return t;
        }
        auto token = *m_it;
        m_it++;
        return token;
    }
    const token_t& peek_token() {
        if(m_peeked.empty()) {
            m_peeked.push_back(*m_it);
            m_it++;
        }
        return m_peeked.back();
    }

    void eat_token() {
        (void)next_token();
    }

    bool eof() {
        if(!m_peeked.empty())
            return detail::is_eof_token(peek_token());
        return (m_it == m_end);
    }

private:
    Rng& m_rng;
    decltype(std::begin(m_rng)) m_it;
    decltype(std::end(m_rng)) m_end;
};

}    // namespace banshee

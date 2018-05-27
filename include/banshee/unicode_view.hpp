#pragma once
#include <memory>
#include <afio/v2.0/afio.hpp>
#include <experimental/filesystem>
#include <banshee/detail/unicode_file.hpp>
#include <cedilla/detail/unicode_base_view.hpp>

namespace afio = AFIO_V2_NAMESPACE;

namespace banshee {

namespace detail {

    class unicode_view_impl_base
        : public ranges::v3::view_facade<unicode_view_impl_base, ranges::finite> {
    public:
        using codepoint = std::experimental::text::unicode_character_set::code_point_type;
        virtual ~unicode_view_impl_base() = default;

        struct cursor_base {
            virtual ~cursor_base() = default;
            virtual codepoint read() const = 0;
            virtual bool equal(ranges::v3::default_sentinel s) const = 0;
            virtual void next() = 0;
            virtual cursor_base* clone() = 0;
        };

        struct cursor {
            cursor() : m_impl(0) {}
            cursor(std::unique_ptr<cursor_base>&& impl) : m_impl(impl.release()) {}
            cursor(cursor&& other) : m_impl(other.m_impl) {
                other.m_impl = nullptr;
            }
            cursor(const cursor& other) : m_impl(other.m_impl->clone()) {}
            ~cursor() {
                delete m_impl;
            }
            cursor& operator=(const cursor& other) {
                m_impl = other.m_impl->clone();
                return *this;
            }
            cursor& operator=(cursor&& other) {
                std::swap(m_impl, other.m_impl);
                return *this;
            }


            codepoint read() const {
                return m_impl->read();
            }
            bool equal(ranges::v3::default_sentinel s) const {
                return !m_impl || m_impl->equal(s);
            }
            void next() {
                return m_impl->next();
            }

        private:
            cursor_base* m_impl;
        };
        cursor begin_cursor() const {
            return do_begin_cursor();
        }
        virtual cursor do_begin_cursor() const {
            return cursor{};
        }
    };


    template<class TV>
    class unicode_view_impl : public unicode_view_impl_base {
    public:
        struct cursor_impl : unicode_view_impl_base::cursor_base {
            using Begin = decltype(std::begin(TV()));
            using End = decltype(std::end(TV()));

            cursor_impl(Begin it, End end) : m_it(it), m_end(end) {}
            codepoint read() const override {
                return *m_it;
            }
            bool equal(ranges::v3::default_sentinel) const override {
                return m_it == m_end;
            }
            void next() override {
                m_it++;
            }
            cursor_impl* clone() override {
                return new cursor_impl(m_it, m_end);
            }

        private:
            Begin m_it;
            End m_end;
        };

        cursor do_begin_cursor() const override {
            return cursor(std::move(
                std::make_unique<cursor_impl>(std::begin(m_text_view), std::end(m_text_view))));
        }

        unicode_view_impl(TV&& tv) : m_text_view(std::move(tv)) {}

    protected:
        TV m_text_view;
    };

    template<class TV>
    class unicode_file_impl : public unicode_view_impl<TV> {
        using file_view = afio::algorithm::mapped_span<typename TV::encoding_type::code_unit_type>;
        using cursor = typename unicode_view_impl_base::cursor;
        using cursor_impl = typename unicode_view_impl<TV>::cursor_impl;

    public:
        unicode_file_impl(afio::mapped_file_handle&& f, file_view&& memory, TV&& tv) :
            unicode_view_impl<TV>(std::move(tv)),
            m_fh(std::move(f)),
            m_memory(std::move(memory)) {}

    private:
        afio::mapped_file_handle m_fh;
        file_view m_memory;
    };

    template<typename TE>
    auto make_unicode_file(afio::mapped_file_handle&& fh)
        -> std::unique_ptr<unicode_view_impl_base> {
        using CT = typename TE::code_unit_type;
        afio::algorithm::mapped_span<CT> memory(fh);
        auto tv = std::experimental::make_text_view<TE>(memory);
        return std::make_unique<unicode_file_impl<decltype(tv)>>(std::move(fh), std::move(memory),
                                                                 std::move(tv));
    }

    template<typename Rng, typename Encoding,
             CONCEPT_REQUIRES_(cedilla::detail::concepts::UtfInputRange<Rng>())>
    auto make_unicode_view_impl(Rng&& rng) {
        using TE = decltype(std::experimental::make_text_view<Encoding>(rng));
        return std::make_unique<unicode_view_impl<TE>>(
            std::move(std::experimental::make_text_view<Encoding>(rng)));
    }
}    // namespace detail

class unicode_view : public ranges::v3::view_facade<unicode_view, ranges::finite> {
    std::unique_ptr<detail::unicode_view_impl_base> m_impl;
    using Begin = decltype(std::begin(*m_impl));
    using End = decltype(std::end(*m_impl));

public:
    using codepoint = typename detail::unicode_view_impl_base::codepoint;
    using value_type = codepoint;
    using iterator = ranges::detail::facade_iterator_t<const detail::unicode_view_impl_base>;
    struct cursor {
        cursor() = default;
        cursor(Begin&& it, End&& end) : m_it(std::move(it)), m_end(std::move(end)) {}

        codepoint read() const {
            return *m_it;
        }
        bool equal(ranges::v3::default_sentinel) const {
            return m_it == m_end;
        }
        void next() {
            return m_it++;
        }

    private:
        Begin m_it;
        End m_end;
    };

    cursor begin_cursor() const {
        return cursor(std::begin(*m_impl), std::end(*m_impl));
    }

    template<typename Rng, CONCEPT_REQUIRES_(cedilla::detail::concepts::CodepointInputRange<Rng>())>
    unicode_view(Rng&& in) :
        m_impl(detail::make_unicode_view_impl<Rng, std::experimental::utf32_encoding>(in)) {}

    template<typename Rng, CONCEPT_REQUIRES_(cedilla::detail::concepts::Utf8InputRange<Rng>())>
    unicode_view(Rng&& in) :
        m_impl(detail::make_unicode_view_impl<Rng, std::experimental::utf8_encoding>(in)) {}

    template<typename Rng, CONCEPT_REQUIRES_(cedilla::detail::concepts::Utf16InputRange<Rng>())>
    unicode_view(Rng&& in) :
        m_impl(detail::make_unicode_view_impl<Rng, std::experimental::utf16_encoding>(in)) {}

    unicode_view(std::unique_ptr<detail::unicode_view_impl_base>&& impl) :
        m_impl(std::move(impl)) {}
};

auto open_unicode_file(std::experimental::filesystem::path path) {
    afio::mapped_file_handle fh = afio::mapped_file({}, path).value();
    afio::algorithm::mapped_span<char> memory(fh);
    std::array<char, 4> bom = {0, 0, 0, 0};
    std::copy(std::begin(memory), std::begin(memory) + std::min(std::size_t{4}, memory.size()),
              std::begin(bom));
    if(detail::test_bom<detail::codec_name::utf8>(bom)) {
        return unicode_view(
            detail::make_unicode_file<std::experimental::utf8bom_encoding>(std::move(fh)));
    }
    if(detail::test_bom<detail::codec_name::utf16LE>(bom) ||
       detail::test_bom<detail::codec_name::utf16BE>(bom)) {
        return unicode_view(
            detail::make_unicode_file<std::experimental::utf16bom_encoding>(std::move(fh)));
    }
    if(detail::test_bom<detail::codec_name::utf32LE>(bom) ||
       detail::test_bom<detail::codec_name::utf32BE>(bom)) {
        return unicode_view(
            detail::make_unicode_file<std::experimental::utf32bom_encoding>(std::move(fh)));
    }
    return unicode_view(detail::make_unicode_file<std::experimental::utf8_encoding>(std::move(fh)));
}


}    // namespace banshee

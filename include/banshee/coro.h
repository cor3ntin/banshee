///////////////////////////////////////////////////////////////////////////////
// Copyright (c) Lewis Baker
// Licenced under MIT license. See LICENSE.txt for details.
///////////////////////////////////////////////////////////////////////////////
#ifndef CPPCORO_GENERATOR_HPP_INCLUDED
#define CPPCORO_GENERATOR_HPP_INCLUDED

#include <experimental/coroutine>
#include <type_traits>
#include <utility>

namespace cppcoro {
template<typename T>
class generator;

namespace detail {
    template<typename T>
    class generator_promise {
    public:
        using value_type = std::remove_reference_t<T>;
        using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;
        using pointer_type = value_type*;

        generator_promise() = default;

        generator<T> get_return_object() noexcept;

        constexpr std::experimental::suspend_always initial_suspend() const {
            return {};
        }
        constexpr std::experimental::suspend_always final_suspend() const {
            return {};
        }

        template<typename U, typename = std::enable_if_t<std::is_same<U, T>::value>>
        std::experimental::suspend_always yield_value(U& value) noexcept {
            m_value = std::addressof(value);
            return {};
        }

        std::experimental::suspend_always yield_value(T&& value) noexcept {
            m_value = std::addressof(value);
            return {};
        }

        void unhandled_exception() {
            std::rethrow_exception(std::current_exception());
        }

        void return_void() {}

        reference_type value() const noexcept {
            return *m_value;
        }

        // Don't allow any use of 'co_await' inside the generator coroutine.
        template<typename U>
        std::experimental::suspend_never await_transform(U&& value) = delete;

    private:
        pointer_type m_value;
    };
}    // namespace detail

template<typename T>
class generator {
public:
    using promise_type = detail::generator_promise<T>;

    generator() noexcept : m_coroutine(nullptr) {}

    generator(generator&& other) noexcept : m_coroutine(other.m_coroutine) {
        other.m_coroutine = nullptr;
    }

    generator(const generator& other) = delete;

    ~generator() {
        if(m_coroutine) {
            m_coroutine.destroy();
        }
    }

    bool has_next() {
        return m_coroutine != nullptr && !m_coroutine.done();
    }

    auto next() {
        if(m_coroutine.done()) {
            m_coroutine = nullptr;
        }
        if(!m_coroutine) {
            return decltype(m_coroutine.promise().value()){};
        }
        m_coroutine.resume();
        return m_coroutine.promise().value();
    }

    generator& operator=(generator other) noexcept {
        swap(other);
        return *this;
    }

    void swap(generator& other) noexcept {
        std::swap(m_coroutine, other.m_coroutine);
    }

private:
    friend class detail::generator_promise<T>;

    explicit generator(std::experimental::coroutine_handle<promise_type> coroutine) noexcept :
        m_coroutine(coroutine) {}

    std::experimental::coroutine_handle<promise_type> m_coroutine;
};

template<typename T>
void swap(generator<T>& a, generator<T>& b) {
    a.swap(b);
}

namespace detail {
    template<typename T>
    generator<T> generator_promise<T>::get_return_object() noexcept {
        using coroutine_handle = std::experimental::coroutine_handle<generator_promise<T>>;
        return generator<T>{coroutine_handle::from_promise(*this)};
    }
}    // namespace detail

template<typename FUNC, typename T>
generator<std::result_of_t<FUNC && (T&)>> fmap(FUNC func, generator<T> source) {
    for(auto& value : source) {
        co_yield std::invoke(func, value);
    }
}
}    // namespace cppcoro

#endif

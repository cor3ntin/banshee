#pragma once

namespace banshee {
namespace detail {
    template<class... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };
    template<class... Ts>
    overloaded(Ts...)->overloaded<Ts...>;
}    // namespace detail
}    // namespace banshee

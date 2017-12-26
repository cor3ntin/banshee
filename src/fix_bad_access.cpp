#include <variant>

namespace std {


const char* bad_variant_access::what() const noexcept {
    return "bad_variant_access";
}

}    // namespace std

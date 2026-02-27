#ifndef STLAB_CHAIN_TRAITS
#define STLAB_CHAIN_TRAITS

#include <stlab/chain/config.hpp>

#include <utility>

namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE() {

template <typename T>
constexpr decltype(auto) move_allow_trivial(T&& t) noexcept {
    return std::move(t);
}
} // namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE()

#endif
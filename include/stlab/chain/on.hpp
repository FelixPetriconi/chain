/*
    Copyright 2026 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef STLAB_CHAIN_ON_HPP
#define STLAB_CHAIN_ON_HPP

#include <stlab/chain/config.hpp>
#include <stlab/chain/segment.hpp>

#include <atomic>
#include <memory>
#include <tuple>
#include <utility>

namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE() {

struct cancellation_source {
    struct state {
        std::atomic_bool canceled{false};
    };
    std::shared_ptr<state> _state = std::make_shared<state>();
    void cancel() const { _state->canceled.store(true, std::memory_order_relaxed); }
};

struct cancellation_token {
    std::shared_ptr<cancellation_source::state> _state;
    [[nodiscard]] auto canceled() const -> bool {
        return _state->canceled.load(std::memory_order_relaxed);
    }
};

// Segment that injects a cancellation_token (Injects != void)
inline auto with_cancellation(cancellation_source src) {
    return segment{stlab::type<cancellation_token>{},
                   [_src = std::move(src)]<typename F, typename... Args>(
                       F&& f, Args&&... args) mutable -> auto {
                       // Create token and forward it as first argument
                       cancellation_token token{_src._state};
                       std::forward<F>(f)(token, std::forward<Args>(args)...);
                   }};
}

// executor variant that also injects the token and schedules asynchronously
template <class E>
auto on_with_cancellation(E&& executor, cancellation_source source) {
    return segment{stlab::type<cancellation_token>{},
                   [_executor = std::forward<E>(executor),
                    _source = std::move(source)]<typename F, typename... Args>(
                       F&& f, Args&&... args) mutable -> auto {
                       cancellation_token token{_source._state};
                       std::move(_executor)([_f = std::forward<F>(f), _token = token,
                                             _args = std::tuple{std::forward<Args>(
                                                 args)...}]() mutable noexcept -> void {
                           std::apply(
                               [&_f, &_token]<typename... As>(As&&... as) -> void {
                                   std::forward<decltype(_f)>(_f)(_token, std::forward<As>(as)...);
                               },
                               std::move(_args));
                       });
                   }};
}
} // namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE()

#endif

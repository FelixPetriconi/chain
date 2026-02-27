/*
    Copyright 2026 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef STLAB_CHAIN_START_HPP
#define STLAB_CHAIN_START_HPP

#include <stlab/chain/config.hpp>

#include <stlab/concurrency/await.hpp>
#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>

namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE() {

template <class Chain, class... Args>
auto start(Chain&& chain, Args&&... args) {
    using result_t = typename Chain::template result_type<Args...>;

    using package_task_t = stlab::packaged_task<result_t>;
    auto shared = std::shared_ptr<package_task_t>();

    // Build the receiver and future first.
    auto [receiver, future] = stlab::package<result_t(result_t)>(
        stlab::immediate_executor,
        [_shared = shared]<typename T>(T&& val) { return std::forward<T>(val); });

    // Promote receiver to shared_ptr to extend lifetime beyond this scope.
    shared = std::make_shared<package_task_t>(std::move(receiver));

    // Recompute invoke_t based on passing the shared_ptr (pointer semantics).
    using invoke_t =
        decltype(std::forward<Chain>(chain).invoke(shared, std::forward<Args>(args)...));

    if constexpr (std::is_void_v<invoke_t>) {
        // Just invoke; lifetime of receiver is now owned by captures inside the async chain.
        std::forward<Chain>(chain).invoke(shared, std::forward<Args>(args)...);
    } else {
        // Keep any handle the chain returns (e.g. continuation future or cancellation handle).
        auto hold = std::forward<Chain>(chain).invoke(shared, std::forward<Args>(args)...);
        (void)hold; // store or return if you later need it
    }
    return std::move(future);
}

} // namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE()

#endif

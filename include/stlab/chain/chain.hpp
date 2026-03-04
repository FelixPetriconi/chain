/*
    Copyright 2026 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#ifndef STLAB_CHAIN_HPP
#define STLAB_CHAIN_HPP

#include <stlab/chain/config.hpp>
#include <stlab/chain/segment.hpp>
#include <stlab/chain/tuple.hpp>

#include <stlab/concurrency/future.hpp>
#include <stlab/concurrency/immediate_executor.hpp>

#include <exception>
#include <tuple>
#include <type_traits>
#include <utility>

#define STLAB_FWD(x) std::forward<decltype(x)>(x)
/*

If exception inside a segment _apply_ function throws an exception then the exception must be
set on the receiver.

*/

namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE() {

namespace detail {

/// Apply a recursive lambda to each element in the tuple-like Segments.
template <class Fold, class Segments>
constexpr auto fold_over(Fold fold, Segments&& segments) -> decltype(auto) {
    return std::apply(
        [fold]<typename... Links>(Links&&... links) mutable {
            return fold(fold, std::forward<Links>(links)...);
        },
        std::forward<decltype(segments)>(segments));
}

} // namespace detail

/*
    simplify this code by handing the multi-argument case earlier (somehow).
*/

template <class Tail, class Injects, class Applicator, class... Fs>
class chain {
    Tail _tail;
    segment<Injects, Applicator, Fs...> _head;

    /// Return a lambda with the signature of
    /// head( tail<n>( tail<1>( tail<0>( auto&& args... ) ) ) )
    /// for computing the result type of this chain.
    static consteval auto result_type_helper(Tail&& tail,
                                             segment<Injects, Applicator, Fs...>&& head) {
        return detail::fold_over(
            []<typename Fold, typename First, typename... Rest>(
                [[maybe_unused]] Fold fold, First&& first, Rest&&... rest) -> auto {
                if constexpr (sizeof...(rest) == 0) {
                    return [_segment = std::forward<First>(first)]<typename... Args>(
                               Args&&... args) mutable -> auto {
                        return std::move(_segment).result_type_helper(std::forward<Args>(args)...);
                    };
                } else {
                    return [_segment = std::forward<First>(first).append(
                                fold(fold, std::forward<Rest>(rest)...))]<typename... Args>(
                               Args&&... args) mutable -> auto {
                        return std::move(_segment).result_type_helper(std::forward<Args>(args)...);
                    };
                }
            },
            std::tuple_cat(std::move(tail), std::make_tuple(std::move(head))));
    }

    template <class R>
    auto expand(R&& receiver) && {
        return detail::fold_over(
            [_receiver =
                 std::forward<R>(receiver)]<typename Fold, typename First, typename... Rest>(
                [[maybe_unused]] Fold fold, First&& first, Rest&&... rest) mutable -> auto {
                if constexpr (sizeof...(rest) == 0) {
                    return [_receiver, _segment = std::forward<First>(first).append(
                                           [_receiver]<typename V>(V&& val) {
                                               if (_receiver) {
                                                   _receiver->operator()(std::forward<V>(val));
                                               }
                                           })]<typename... Args>(Args&&... args) mutable -> auto {
                        return std::move(_segment).invoke(_receiver, std::forward<Args>(args)...);
                    };
                } else {
                    return [_receiver, _segment = std::forward<First>(first).append(fold(
                                           fold, std::forward<Rest>(rest)...))]<typename... Args>(
                               Args&&... args) mutable -> auto {
                        return std::move(_segment).invoke(_receiver, std::forward<Args>(args)...);
                    };
                }
            },
            std::tuple_cat(std::move(_tail), std::make_tuple(std::move(_head))));
    }

    template <class... Args>
    struct result_type_void_injects {
        using type = decltype(result_type_helper(
            std::declval<Tail>(),
            std::declval<segment<Injects, Applicator, Fs...>>())(std::declval<Args>()...));
    };

    template <class... Args>
    struct result_type_injects {
        using type = decltype(result_type_helper(
            std::declval<Tail>(), std::declval<segment<Injects, Applicator, Fs...>>())(
            std::declval<Injects>(), std::declval<Args>()...));
    };

public:
    template <class... Args>
    using result_type = std::conditional_t<std::is_same_v<Injects, void>,
                                           result_type_void_injects<Args...>,
                                           result_type_injects<Args...>>::type;

    explicit chain(Tail&& tail, segment<Injects, Applicator, Fs...>&& head)
        : _tail{std::move(tail)}, _head{std::move(head)} {}

    /*
        The basic operations should follow those from C++ lambdas, for now default everything.
        and see if the compiler gets it correct.
    */

    chain(const chain&) = default;
    chain(chain&&) noexcept = default;
    auto operator=(const chain&) -> chain& = default;
    auto operator=(chain&&) noexcept -> chain& = default;

    // append function to the last sequence
    template <class F>
    auto append(F&& f) && {
        // the namespace specifier is needed, otherwise the compiler fails.
        return stlab::chain{std::move(_tail), std::move(_head).append(std::forward<F>(f))};
    }

    template <class Jnjects, class I, class... Gs>
    auto append(segment<Jnjects, I, Gs...>&& head) && {
        // the namespace specifier is needed, otherwise the compiler fails.
        return stlab::chain{std::tuple_cat(std::move(_tail), std::make_tuple(std::move(_head))),
                            std::move(head)};
    }

    template <class Receiver, class... Args>
    auto invoke(Receiver&& receiver, Args&&... args) && {
        return std::move(*this).expand(std::forward<Receiver>(receiver))(
            std::forward<Args>(args)...);
    }

    template <class F>
    friend auto operator|(chain&& c, F&& f) {
        return std::move(c).append(std::forward<F>(f));
    }

    template <class Jnjects, class I, class... Gs>
    friend auto operator|(chain&& c, segment<Jnjects, I, Gs...>&& head) {
        return std::move(c).append(std::move(head));
    }
};

template <class Tail, class Injects, class Applicator, class... Fs>
chain(Tail&& tail, segment<Injects, Applicator, Fs...>&& head)
    -> chain<Tail, Injects, Applicator, Fs...>;

template <class F, class Injects, class Applicator, class... Fs>
auto operator|(segment<Injects, Applicator, Fs...>&& head, F&& f) {
    return chain{std::tuple<>{}, std::move(head).append(std::forward<F>(f))};
}

/*

Each segment invokes the next segment with result and returns void. Promise is bound to the
last item in the chain as a segment.

*/
template <class E>
auto on(E&& executor) {
    return segment{
        type<void>{}, [_executor = std::forward<E>(executor)]<typename F, typename... Args>(
                          F&& f, Args&&... args) mutable {
            std::move(_executor)(
                [_f = std::forward<F>(f),
                 _args = std::make_tuple(std::forward<Args>(args)...)]() mutable noexcept {
                    std::apply(std::move(_f), std::move(_args));
                });
            // return std::monostate{};
        }};
}

} // namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE()

#endif

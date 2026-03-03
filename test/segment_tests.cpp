/*
    Copyright 2026 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#include "stlab/chain/traits.hpp"

#include <stlab/chain/segment.hpp>

#include <doctest/doctest.h>
#include <stlab/test/model.hpp> // moveonly

#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>

// Mock receiver for testing invoke
struct mock_receiver {
    bool _canceled{false};
    std::exception_ptr _exception;
    int _result{0};

    [[nodiscard]] auto canceled() const -> bool { return _canceled; }
    auto set_exception(const std::exception_ptr& e) -> void { _exception = e; }
    auto set_value(int value) -> void { _result = value; }
};

TEST_CASE("[segment] Basic segment operations") {
    SUBCASE("simple creation with variadic constructor") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  []() -> int { return 42; }};
        (void)sut;
    }

    SUBCASE("simple creation with tuple constructor") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  std::make_tuple([]() -> int { return 42; })};
        (void)sut;
    }

    SUBCASE("creation with multiple functions") {
        auto sut =
            stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                           [](int x) -> int { return x + 1; }, [](int x) -> int { return x * 2; }};
        (void)sut;
    }

    SUBCASE("creation with empty function tuple") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  std::tuple<>{}};
        (void)sut;
    }
}

TEST_CASE("[segment] Segment copy and move semantics") {
    SUBCASE("copy constructor") {
        auto original = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                       []() -> int { return 42; }};
        auto copy{original};

        auto original_result = std::move(original).result_type_helper();
        CHECK(original_result == 42);
        auto copy_result = std::move(copy).result_type_helper();
        CHECK(copy_result == 42);
    }

    SUBCASE("move constructor") {
        auto reference = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                        []() -> int { return 42; }};

        auto original = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                       []() -> int { return 42; }};

        auto moved = std::move(original);
        auto moved_result = std::move(moved).result_type_helper();
        CHECK(moved_result == 42);
        auto reference_result = std::move(reference).result_type_helper();
        CHECK(reference_result == 42);
    }

    SUBCASE("segment with move-only types") {
        auto reference = stlab::segment{
            stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
            [m = stlab::move_only(42)]() mutable -> stlab::move_only { return std::move(m); }};
        auto original = stlab::segment{
            stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
            [m = stlab::move_only(42)]() mutable -> stlab::move_only { return std::move(m); }};

        auto moved = std::move(original);
        auto moved_result = std::move(moved).result_type_helper();
        CHECK(moved_result == stlab::move_only(42));
        auto reference_result = std::move(reference).result_type_helper();
        CHECK(reference_result == stlab::move_only(42));
    }
}

TEST_CASE("[segment] Segment result_type_helper") {
    SUBCASE("single function returning int") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  []() -> int { return 42; }};
        auto result = std::move(sut).result_type_helper();
        CHECK(result == 42);
    }

    SUBCASE("function chain with transformations") {
        auto sut = stlab::segment{
            stlab::type<std::tuple<>>{}, [](auto f, auto... args) -> void { f(args...); },
            [](int x) -> int { return x + 1; }, [](int x) -> int { return x * 2; }};
        auto result = std::move(sut).result_type_helper(5);
        CHECK(result == 12); // (5 + 1) * 2 = 12
    }

    SUBCASE("function chain returning string") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [](int x) -> int { return x * 2; },
                                  [](int x) -> std::string { return std::to_string(x); }};
        auto result = std::move(sut).result_type_helper(21);
        CHECK(result == "42");
    }

    SUBCASE("void returning function") {
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [&hit](int x) -> void { hit = x; }};
        std::move(sut).result_type_helper(42);
        CHECK(hit == 42);
    }
}

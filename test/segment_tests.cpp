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
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  []() -> int { return 42; }};
        auto result = std::move(sut).result_type_helper();
        CHECK(result == 42);
    }

    SUBCASE("function chain with transformations") {
        auto sut =
            stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                           [](int x) -> int { return x + 1; }, [](int x) -> int { return x * 2; }};
        auto result = std::move(sut).result_type_helper(5);
        CHECK(result == 12); // (5 + 1) * 2 = 12
    }

    SUBCASE("function chain returning string") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [](int x) -> int { return x * 2; },
                                  [](int x) -> std::string { return std::to_string(x); }};
        auto result = std::move(sut).result_type_helper(21);
        CHECK(result == "42");
    }

    SUBCASE("void returning function") {
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [&hit](int x) -> void { hit = x; }};
        std::move(sut).result_type_helper(42);
        CHECK(hit == 42);
    }
}

TEST_CASE("[segment] Segment invoke with receiver") {
    SUBCASE("invoke with non-canceled receiver") {
        auto receiver = std::make_shared<mock_receiver>();
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [&hit](int x) -> void { hit = x; }};

        std::move(sut).invoke(receiver, 42);
        CHECK(hit == 42);
        CHECK(receiver->_exception == nullptr);
    }

    SUBCASE("invoke with canceled receiver") {
        auto receiver = std::make_shared<mock_receiver>();
        receiver->_canceled = true;
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [&hit](int x) -> void { hit = x; }};

        std::move(sut).invoke(receiver, 42);
        CHECK(hit == 0); // Should not execute
    }

    SUBCASE("invoke with exception in segment") {
        auto receiver = std::make_shared<mock_receiver>();
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [](int x) -> int {
                                      if (x == 42) throw std::runtime_error("test error");
                                      return x;
                                  }};

        std::move(sut).invoke(receiver, 42);
        CHECK(receiver->_exception != nullptr);

        bool caught_exception = false;
        try {
            std::rethrow_exception(receiver->_exception);
        } catch (const std::runtime_error& e) {
            caught_exception = true;
            CHECK(std::string(e.what()) == "test error");
        }
        CHECK(caught_exception);
    }

    SUBCASE("invoke with applicator that modifies behavior") {
        auto receiver = std::make_shared<mock_receiver>();
        auto hit = 0;

        // Custom applicator that doubles the argument
        auto custom_apply = [](auto f, int x) -> void { f(x * 2); };

        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, std::move(custom_apply),
                                  [&hit](int x) -> void { hit = x; }};

        std::move(sut).invoke(receiver, 21);
        CHECK(hit == 42); // 21 * 2 = 42
    }

    SUBCASE("invoke with chained functions") {
        auto receiver = std::make_shared<mock_receiver>();
        auto result = 0;
        auto sut = stlab::segment{
            stlab::type<std::tuple<>>{}, [](auto f, auto... args) -> void { f(args...); },
            [](int x) -> int { return x + 1; }, [](int x) -> int { return x * 2; },
            [&result](int x) -> void { result = x; }};

        std::move(sut).invoke(receiver, 5);
        CHECK(result == 12); // (5 + 1) * 2 = 12
        CHECK(receiver->_exception == nullptr);
    }
}

TEST_CASE("[segment] Segment with injected types") {
    SUBCASE("segment with int injection") {
        auto sut = stlab::segment{stlab::type<std::tuple<int>>{}, [](auto f) -> void { f(); },
                                  []() -> int { return 42; }};
        // Segment should be constructible with injection type
        (void)sut;
    }

    SUBCASE("segment with multiple injection types") {
        auto sut = stlab::segment{stlab::type<std::tuple<int, std::string>>{},
                                  [](auto f) -> void { f(); }, []() -> int { return 42; }};
        // Segment should be constructible with multiple injection types
        (void)sut;
    }
}

TEST_CASE("[segment] Segment edge cases") {
    SUBCASE("empty segment with no functions") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  std::tuple<>{}};
        // Should be constructible
        (void)sut;
    }

    SUBCASE("segment with void function") {
        auto hit = false;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [&hit]() -> void { hit = true; }};
        std::move(sut).result_type_helper();
        CHECK(hit);
    }

    SUBCASE("segment with multiple void functions") {
        auto hit1 = false;
        auto hit2 = false;
        auto sut =
            stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                           [&hit1]() -> void { hit1 = true; }, [&hit2]() -> void { hit2 = true; }};
        std::move(sut).result_type_helper();
        CHECK(hit1);
        CHECK(hit2);
    }

    SUBCASE("segment with variadic function") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [](auto... args) -> int { return (args + ...); }};
        auto result = std::move(sut).result_type_helper(1, 2, 3, 4);
        CHECK(result == 10);
    }
}

TEST_CASE("segment with pair<int, string>") {
    SUBCASE("pair<int, string>(int, string) -> pair<int, string>(pair<int, string>)") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](const std::pair<int, std::string>& v) { return v; }};

        auto result = stlab::move_allow_trivial(sut).result_type_helper(
            std::make_pair(42, std::string("test")));
        CHECK(result == std::make_pair(42, std::string("test")));
    }
    SUBCASE("pair<int, string>(int, string) -> pair<int, string>(pair<int, string>)") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](const std::pair<int, std::string>& v) { return v; },
                                  [](const std::pair<int, std::string>& p) {
                                      return p.second + "=" + std::to_string(p.first);
                                  }};

        auto result = stlab::move_allow_trivial(sut).result_type_helper(
            std::make_pair(42, std::string("test")));
        CHECK(result == std::string("test=42"));
    }
}

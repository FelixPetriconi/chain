/*
    Copyright 2026 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#include <stlab/segment.hpp>

#include <catch2/catch_test_macros.hpp>
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

TEST_CASE("Basic segment operations", "[segment]") {
    SECTION("simple creation with variadic constructor") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  []() -> int { return 42; }};
        (void)sut;
    }

    SECTION("simple creation with tuple constructor") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  std::make_tuple([]() -> int { return 42; })};
        (void)sut;
    }

    SECTION("creation with multiple functions") {
        auto sut =
            stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                           [](int x) -> int { return x + 1; }, [](int x) -> int { return x * 2; }};
        (void)sut;
    }

    SECTION("creation with empty function tuple") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  std::tuple<>{}};
        (void)sut;
    }
}

TEST_CASE("Segment copy and move semantics", "[segment]") {
    SECTION("copy constructor") {
        auto original = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                       []() -> int { return 42; }};
        auto copy{original};

        auto original_result = std::move(original).result_type_helper();
        CHECK(original_result == 42);
        auto copy_result = std::move(copy).result_type_helper();
        CHECK(copy_result == 42);
    }

    SECTION("move constructor") {
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

    SECTION("segment with move-only types") {
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

TEST_CASE("Segment result_type_helper", "[segment]") {
    SECTION("single function returning int") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  []() -> int { return 42; }};
        auto result = std::move(sut).result_type_helper();
        CHECK(result == 42);
    }

    SECTION("function chain with transformations") {
        auto sut =
            stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                           [](int x) -> int { return x + 1; }, [](int x) -> int { return x * 2; }};
        auto result = std::move(sut).result_type_helper(5);
        CHECK(result == 12); // (5 + 1) * 2 = 12
    }

    SECTION("function chain returning string") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [](int x) -> int { return x * 2; },
                                  [](int x) -> std::string { return std::to_string(x); }};
        auto result = std::move(sut).result_type_helper(21);
        CHECK(result == "42");
    }

    SECTION("void returning function") {
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [&hit](int x) -> void { hit = x; }};
        std::move(sut).result_type_helper(42);
        CHECK(hit == 42);
    }
}

TEST_CASE("Segment invoke with receiver", "[segment]") {
    SECTION("invoke with non-canceled receiver") {
        auto receiver = std::make_shared<mock_receiver>();
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [&hit](int x) -> void { hit = x; }};

        std::move(sut).invoke(receiver, 42);
        CHECK(hit == 42);
        CHECK(receiver->_exception == nullptr);
    }

    SECTION("invoke with canceled receiver") {
        auto receiver = std::make_shared<mock_receiver>();
        receiver->_canceled = true;
        auto hit = 0;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{},
                                  [](auto f, auto... args) -> void { f(args...); },
                                  [&hit](int x) -> void { hit = x; }};

        std::move(sut).invoke(receiver, 42);
        CHECK(hit == 0); // Should not execute
    }

    SECTION("invoke with exception in segment") {
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

    SECTION("invoke with applicator that modifies behavior") {
        auto receiver = std::make_shared<mock_receiver>();
        auto hit = 0;

        // Custom applicator that doubles the argument
        auto custom_apply = [](auto f, int x) -> void { f(x * 2); };

        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, std::move(custom_apply),
                                  [&hit](int x) -> void { hit = x; }};

        std::move(sut).invoke(receiver, 21);
        CHECK(hit == 42); // 21 * 2 = 42
    }

    SECTION("invoke with chained functions") {
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

TEST_CASE("Segment with injected types", "[segment]") {
    SECTION("segment with int injection") {
        auto sut = stlab::segment{stlab::type<std::tuple<int>>{}, [](auto f) -> void { f(); },
                                  []() -> int { return 42; }};
        // Segment should be constructible with injection type
        (void)sut;
    }

    SECTION("segment with multiple injection types") {
        auto sut = stlab::segment{stlab::type<std::tuple<int, std::string>>{},
                                  [](auto f) -> void { f(); }, []() -> int { return 42; }};
        // Segment should be constructible with multiple injection types
        (void)sut;
    }
}

TEST_CASE("Segment edge cases", "[segment]") {
    SECTION("empty segment with no functions") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  std::tuple<>{}};
        // Should be constructible
        (void)sut;
    }

    SECTION("segment with void function") {
        auto hit = false;
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [&hit]() -> void { hit = true; }};
        std::move(sut).result_type_helper();
        CHECK(hit);
    }

    SECTION("segment with multiple void functions") {
        auto hit1 = false;
        auto hit2 = false;
        auto sut =
            stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                           [&hit1]() -> void { hit1 = true; }, [&hit2]() -> void { hit2 = true; }};
        std::move(sut).result_type_helper();
        CHECK(hit1);
        CHECK(hit2);
    }

    SECTION("segment with variadic function") {
        auto sut = stlab::segment{stlab::type<std::tuple<>>{}, [](auto f) -> void { f(); },
                                  [](auto... args) -> int { return (args + ...); }};
        auto result = std::move(sut).result_type_helper(1, 2, 3, 4);
        CHECK(result == 10);
    }
}

#include <chains/tuple.hpp>

#include <catch2/catch_test_macros.hpp>
#include <stlab/test/model.hpp> // moveonly

#include <cstddef>
#include <string>
#include <tuple>
#include <utility>

TEST_CASE("Test tuple compose", "[tuple]") {
    std::tuple t{[](int x) { return x + 1.0; }, [](double x) { return x * 2.0; },
                 [](double x) { return std::to_string(x / 2.0); }};
    auto f = chains::tuple_compose(std::move(t));
    REQUIRE(f(1) == "2.000000");
}

struct multi_callable {
    auto operator()(int a, float b) const -> int { return a + static_cast<int>(b); }
    auto operator()(int a, float b, int c) const -> int { return a + static_cast<int>(b) + c; }
};

struct void_t {};

auto oneInt2Int = [](int a) -> int { return a; };
// auto twoInt2Int = [](int a, int b) { return a + b; };
// auto void2Int = []() { return 42; };
auto string2Int = [](const std::string& s) -> std::size_t { return s.size(); };
auto oneInt2String = [](int a) -> std::string { return std::to_string(a); };
auto moveonly2Int = [](stlab::move_only m) -> int { return m.member(); };
auto oneInt2Moveonly = [](int a) -> stlab::move_only { return {a}; };

TEST_CASE("Test tuple consume", "[tuple]") {
    GIVEN("A tuple with a single value") {
        WHEN("an int value is passed into a function that returns an int") {
            std::tuple t{42};
            THEN("this value is returned") {
                auto result = chains::tuple_consume(t)(oneInt2Int);
                REQUIRE(result == std::make_tuple(42));
            }
        }
        WHEN("an int value is passed into a function that returns a string") {
            std::tuple t{42};
            THEN("this value is returned") {
                auto result = chains::tuple_consume(t)(oneInt2String);
                REQUIRE(result == std::make_tuple(std::string("42")));
            }
        }
        WHEN("a string value is passed into a function that returns an int") {
            std::tuple t{std::string("42")};
            THEN("this value is returned") {
                auto result = chains::tuple_consume(t)(string2Int);
                REQUIRE(result == std::make_tuple(2));
            }
        }
        WHEN("an int value is passed into a function that returns a move-only type") {
            std::tuple t{42};
            THEN("this value is returned") {
                auto result = chains::tuple_consume(t)(oneInt2Moveonly);
                REQUIRE(result == std::make_tuple(stlab::move_only(42)));
            }
        }
        WHEN("a move-only value is passed into a function that returns an int") {
            std::tuple t{stlab::move_only(42)};
            THEN("this value is returned") {
                auto result = chains::tuple_consume(std::move(t))(moveonly2Int);
                REQUIRE(result == std::make_tuple(42));
            }
        }
    }
    //
    //
    // GIVEN("A tuple of mixed types")
    // {
    //     std::tuple t{1, 2.0f, 3, 4.0f};
    //
    //     THEN("it returns the correct result for a lambda with two parameters")
    //     {
    //         auto func = [](int a, float b) { return a + static_cast<int>(b); };
    //         auto result = chains::tuple_consume(t)(func);
    //         REQUIRE((result == std::make_tuple(3, 3, 4.0f)));
    //     }
    //     THEN("it returns the correct result for a lambda with no parameters") {
    //         auto func = []() { return 42; };
    //         auto result = chains::tuple_consume(t)(func);
    //         REQUIRE((result == std::make_tuple(42, 1, 2.0f, 3, 4.0f)));
    //     }
    //     THEN("it returns the correct result for a callable with various call operators") {
    //         auto result = chains::tuple_consume(t)(multi_callable{});
    //         REQUIRE((result == std::make_tuple(6, 4.0f)));
    //     }
    // }
}

//
// TEST_CASE("Test concat functions", "[tuple_consume]") {
//
//     GIVEN("A tuple of mixed types")
//     {
//
//         auto functions = std::make_tuple(oneInt2Int, void2Int, twoInt2Int);
//
//         GIVEN("the calculation is done")
//         {
//             auto result = chains::calc(functions, 2);
//             REQUIRE(46 == result);
//         }
//     }
// }
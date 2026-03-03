#include <stlab/chain/chain.hpp>
#include <stlab/chain/on.hpp>
#include <stlab/chain/start.hpp>

#include <doctest/doctest.h>

#include <stlab/concurrency/immediate_executor.hpp>

#include <cctype>
#include <iostream>
#include <string>
#include <utility>

using namespace stlab;

// ============================================================================
// Basic Chain Operations Tests
// ============================================================================

TEST_CASE("[chain] Basic chain operations") {
    SUBCASE("Chain with single lambda") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; };
        auto f = start(std::move(a0), 42);
        REQUIRE(f.get_ready() == 84);
    }

    SUBCASE("Chain with two lambdas") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return std::to_string(x); } | on(immediate_executor) |
                  [](const std::string& s) { return s + "!"; };

        auto f = start(std::move(a0), 42);
        auto val = f.get_ready();
        REQUIRE(val == std::string("84!"));
    }

    SUBCASE("Chain with three lambdas") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 10; } | on(immediate_executor) |
                  [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x - 5; };

        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 25); // ((5+10)*2)-5
    }
}

// ============================================================================
// Type Conversion Tests
// ============================================================================

TEST_CASE("[chain] Type conversions in chain") {
    SUBCASE("int to string") {
        auto a0 = on(immediate_executor) | [](int x) { return std::to_string(x); };
        auto f = start(std::move(a0), 123);
        REQUIRE(f.get_ready() == std::string("123"));
    }

    SUBCASE("string to int") {
        auto a0 = on(immediate_executor) |
                  [](const std::string& s) { return static_cast<int>(s.length()); };
        auto f = start(std::move(a0), std::string("hello"));
        REQUIRE(f.get_ready() == 5);
    }

    SUBCASE("int to double") {
        auto a0 = on(immediate_executor) | [](int x) { return static_cast<double>(x) * 1.5; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 15.0);
    }

    SUBCASE("double to int") {
        auto a0 = on(immediate_executor) | [](double x) { return static_cast<int>(x); };
        auto f = start(std::move(a0), 3.7);
        REQUIRE(f.get_ready() == 3);
    }
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_CASE("[chain] Arithmetic operations in chain") {
    SUBCASE("Mixed arithmetic operations") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x + 10; } | on(immediate_executor) |
                  [](int x) { return x / 2; };
        auto f = start(std::move(a0), 8);
        REQUIRE(f.get_ready() == 13); // ((8*2)+10)/2
    }

    SUBCASE("Modulo operations") {
        auto a0 = on(immediate_executor) | [](int x) { return x % 5; };
        auto f = start(std::move(a0), 17);
        REQUIRE(f.get_ready() == 2);
    }
}

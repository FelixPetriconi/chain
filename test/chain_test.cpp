#include <stlab/chain/start.hpp>

#include "stlab/chain/chain.hpp"
#include "test_helper.hpp"

#include <doctest/doctest.h>

#include <stlab/concurrency/default_executor.hpp>
#include <stlab/concurrency/immediate_executor.hpp>

#include <cctype>
#include <iostream>
#include <string>
#include <utility>

using namespace stlab;
using namespace stlab::test_helper;

// ============================================================================
// Basic Chain Operations Tests
// ============================================================================

TEST_CASE("[chain] Basic chain operations") {
    SUBCASE("Chain with single lambda") {
        auto a0 = on(immediate_executor) | intTimes2{};
        auto f = start(std::move(a0), 42);
        REQUIRE(f.get_ready() == 84);
    }

    SUBCASE("Chain with two lambdas") {
        auto a0 = on(immediate_executor) | intTimes2{} | on(immediate_executor) | int2String{} |
                  on(immediate_executor) | [](const std::string& s) { return s + "!"; };

        auto f = start(std::move(a0), 42);
        auto val = f.get_ready();
        REQUIRE(val == std::string("84!"));
    }

    SUBCASE("Chain with three lambdas") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 10; } | on(immediate_executor) |
                  intTimes2{} | on(immediate_executor) | [](int x) { return x - 5; };

        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 25); // ((5+10)*2)-5
    }
}

// ============================================================================
// Type Conversion Tests
// ============================================================================

TEST_CASE("[chain] Type conversions in chain") {
    SUBCASE("int to string") {
        auto a0 = on(immediate_executor) | int2String{};
        auto f = start(std::move(a0), 123);
        REQUIRE(f.get_ready() == std::string("123"));
    }

    SUBCASE("string to int") {
        auto a0 = on(immediate_executor) | string2Int{};
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
    SUBCASE("Addition chain") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 5; } | on(immediate_executor) |
                  [](int x) { return x + 3; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 18);
    }

    SUBCASE("Multiplication chain") {
        auto a0 = on(immediate_executor) | intTimes2{} | on(immediate_executor) |
                  [](int x) { return x * 3; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 30);
    }

    SUBCASE("Mixed arithmetic operations") {
        auto a0 = on(immediate_executor) | intTimes2{} | on(immediate_executor) |
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

// ============================================================================
// String Operations Tests
// ============================================================================

TEST_CASE("[chain] String operations in chain") {
    SUBCASE("String concatenation") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s + " World"; };
        auto f = start(std::move(a0), std::string("Hello"));
        REQUIRE(f.get_ready() == std::string("Hello World"));
    }

    SUBCASE("String length operation") {
        auto a0 = on(immediate_executor) | string2Int{};
        auto f = start(std::move(a0), std::string("test"));
        REQUIRE(f.get_ready() == 4);
    }

    SUBCASE("Chain string transformations") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s + "!"; } |
                  on(immediate_executor) | [](const std::string& s) { return s + "!"; };
        auto f = start(std::move(a0), std::string("Hi"));
        REQUIRE(f.get_ready() == std::string("Hi!!"));
    }

    SUBCASE("String to uppercase pattern") {
        auto a0 = on(immediate_executor) | [](const std::string& s) {
            std::string result = s;
            for (auto& c : result) {
                c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            }
            return result;
        };
        auto f = start(std::move(a0), std::string("hello"));
        REQUIRE(f.get_ready() == std::string("HELLO"));
    }
}

// ============================================================================
// Boolean/Conditional Logic Tests
// ============================================================================

TEST_CASE("[chain] Conditional logic in chain") {
    SUBCASE("Simple conditional") {
        auto a0 = on(immediate_executor) | [](int x) { return x > 10 ? 100 : 50; };
        auto f = start(std::move(a0), 15);
        REQUIRE(f.get_ready() == 100);
    }

    SUBCASE("Conditional returning false") {
        auto a0 = on(immediate_executor) | [](int x) { return x > 10 ? 100 : 50; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 50);
    }

    SUBCASE("Chain with conditional transformation") {
        auto a0 = on(immediate_executor) | intTimes2{} | on(immediate_executor) |
                  [](int x) { return x > 50 ? true : false; };
        auto f = start(std::move(a0), 30);
        REQUIRE(f.get_ready() == true);
    }

    SUBCASE("Multiple conditional chains") {
        auto a0 = on(immediate_executor) | [](int x) { return x > 0 ? x : -x; } |
                  on(immediate_executor) | [](int x) { return x < 10 ? 10 : x; };
        auto f = start(std::move(a0), -5);
        REQUIRE(f.get_ready() == 10);
    }
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST_CASE("[chain] Edge cases and boundary conditions") {
    SUBCASE("Zero as input") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 0; };
        auto f = start(std::move(a0), 0);
        REQUIRE(f.get_ready() == 0);
    }

    SUBCASE("Negative numbers") {
        auto a0 = on(immediate_executor) | [](int x) { return x * -1; };
        auto f = start(std::move(a0), -5);
        REQUIRE(f.get_ready() == 5);
    }

    SUBCASE("Large numbers") {
        auto a0 = on(immediate_executor) | [](long x) { return x + 1000000L; };
        auto f = start(std::move(a0), 9000000L);
        REQUIRE(f.get_ready() == 10000000L);
    }

    SUBCASE("Empty string") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s.empty(); };
        auto f = start(std::move(a0), std::string(""));
        REQUIRE(f.get_ready() == true);
    }

    SUBCASE("Single character string") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s.length(); };
        auto f = start(std::move(a0), std::string("a"));
        REQUIRE(f.get_ready() == 1);
    }

    SUBCASE("Chain with identity function") {
        auto a0 = on(immediate_executor) | [](int x) { return x; };
        auto f = start(std::move(a0), 42);
        REQUIRE(f.get_ready() == 42);
    }

    SUBCASE("Chain with pair<int,int> argument") {
        auto sut = on(immediate_executor) | [](const auto& v) {
            return v;
        } | [](const std::pair<int, int>& v) { return std::make_pair(v.first * 2, v.second * 3); };
        auto f = start(std::move(sut), std::make_pair(3, 2));
        CHECK(f.get_ready() == std::make_pair(6, 6));
    }

    SUBCASE("Chain with tuple<int, string> argument") {
        auto sut = on(immediate_executor) | [](const auto& v) { return v; } |
                   [](const std::tuple<int, int, int>& v) {
                       return std::make_tuple(std::get<0>(v) * 2, std::get<1>(v) * std::get<2>(v));
                   };
        auto f = start(std::move(sut), std::make_tuple(1, 2, 3));
        CHECK(f.get_ready() == std::make_tuple(2, 6));
    }
    SUBCASE("Chain with many functions") {
        auto sut = on(immediate_executor) | [](const auto& v) { return v; } |
                   [](const auto& v) { return v + 1; } | []() { return 42; } |
                   [](int a, int b) { return a + b; } | []() { return 13; } | []() { return 15; } |
                   [](int a) { return a + 10; } |
                   [](int a, int b, int c) { return std::make_pair(a + b, c); };

        auto f = start(std::move(sut), 3);
        CHECK(f.get_ready() == std::make_pair(38, 46));
    }
}

// ============================================================================
// Future-Related Tests
// ============================================================================

TEST_CASE("[chain] Future operations") {
    SUBCASE("Future is ready after start") {
        auto a0 = on(immediate_executor) | intTimes2{};
        auto f = start(std::move(a0), 10);
        REQUIRE(f.is_ready());
    }

    SUBCASE("get_ready returns correct value") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 5; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 15);
    }

    SUBCASE("Multiple chain invocations with different inputs") {
        auto a0 = on(immediate_executor) | intTimes2{};

        auto f1 = start(std::move(a0), 5);
        REQUIRE(f1.get_ready() == 10);

        auto a1 = on(immediate_executor) | intTimes2{};
        auto f2 = start(std::move(a1), 10);
        REQUIRE(f2.get_ready() == 20);
    }
}

// ============================================================================
// Functional Programming Tests
// ============================================================================

TEST_CASE("[chain] Functional programming patterns") {
    SUBCASE("Map-like operation") {
        auto a0 = on(immediate_executor) | intTimes2{} | on(immediate_executor) |
                  [](int x) { return x + 1; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 11); // (5*2)+1
    }

    SUBCASE("Filter-like operation via conditional") {
        auto a0 = on(immediate_executor) | [](int x) { return (x % 2 == 0) ? x : 0; };
        auto f = start(std::move(a0), 4);
        REQUIRE(f.get_ready() == 4);
    }

    SUBCASE("Filter-like operation removing odd") {
        auto a0 = on(immediate_executor) | [](int x) { return (x % 2 == 0) ? x : 0; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 0);
    }

    SUBCASE("Fold-like operation in chain") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 10; } | on(immediate_executor) |
                  intTimes2{} | on(immediate_executor) | [](int x) { return x - 5; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 25); // ((5+10)*2)-5
    }
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST_CASE("[chain] Complex types in chain") {
    SUBCASE("Return pair") {
        auto a0 = on(immediate_executor) | [](int x) { return std::make_pair(x, x * 2); };
        auto f = start(std::move(a0), 5);
        auto result = f.get_ready();
        REQUIRE(result.first == 5);
        REQUIRE(result.second == 10);
    }

    SUBCASE("Chain with tuple-like operations concatenated") {
        auto a0 = on(immediate_executor) |
                  [](int x) { return std::make_pair(x, std::to_string(x)); } |
                  on(immediate_executor) | [](const std::pair<int, std::string>& p) {
                      return p.second + "=" + std::to_string(p.first);
                  };
        auto f = start(std::move(a0), 42);
        REQUIRE(f.get_ready() == std::string("42=42"));
    }
}

// ============================================================================
// Cancellation Tests
// ============================================================================

TEST_CASE("[chain] Cancellation functionality") {
    SUBCASE("Cancellation before start through future") {
        std::mutex block;
        bool hit = false;
        block.lock(); // Block the chain execution
        auto a0 = on(default_executor) |
                  [&](int x) {
                      std::unique_lock lock(block);
                      return x * 2;
                  } |
                  on(immediate_executor) | [&](int x) {
                      hit = true;
                      return x + 10;
                  };
        auto f = start(std::move(a0), 5);
        f.reset();      // Cancel the future before it executes
        block.unlock(); // Unblock the chain execution

        REQUIRE(hit == false);
    }
}

// ============================================================================
// Multi-Stage Pipeline Tests
// ============================================================================

TEST_CASE("[chain] Multi-stage pipelines") {
    SUBCASE("Five-stage pipeline") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 1; } | on(immediate_executor) |
                  intTimes2{} | on(immediate_executor) | [](int x) { return x - 3; } |
                  on(immediate_executor) | [](int x) { return x / 2; } | on(immediate_executor) |
                  [](int x) { return x + 10; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 19); // (((10+1)*2-3)/2)+10
    }

    SUBCASE("Type-changing pipeline") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 5; } | on(immediate_executor) |
                  int2String{} | on(immediate_executor) |
                  [](const std::string& s) { return s + "!"; } | on(immediate_executor) |
                  [](const std::string& s) { return s.length(); };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 3); // "15!" has length 3
    }
}

// ============================================================================
// Double/Float Operations Tests
// ============================================================================

TEST_CASE("[chain] Floating-point operations") {
    SUBCASE("Simple double multiplication") {
        auto a0 = on(immediate_executor) | [](double x) { return x * 2.5; };
        auto f = start(std::move(a0), 4.0);
        REQUIRE(f.get_ready() == 10.0);
    }

    SUBCASE("Double precision chain") {
        auto a0 = on(immediate_executor) | [](double x) { return x + 0.1; } |
                  on(immediate_executor) | [](double x) { return x * 2.0; };
        auto f = start(std::move(a0), 1.0);
        REQUIRE(f.get_ready() == 2.2);
    }

    SUBCASE("Float to int conversion") {
        auto a0 = on(immediate_executor) | [](float x) { return x * 2.0f; } |
                  on(immediate_executor) | [](float x) { return static_cast<int>(x); };
        auto f = start(std::move(a0), 2.5f);
        REQUIRE(f.get_ready() == 5);
    }
}

// ============================================================================
// Cancellation Token Tests
// ============================================================================

TEST_CASE("[chain] Cancellation token functionality") {
    SUBCASE("Cancellation token is not canceled by default") {
        bool was_canceled = false;
        cancellation_point token;

        auto a0 = on(token) | [&](cancellation_point token, int x) {
            was_canceled = token.canceled();
            return x * 2;
        };

        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 10);
        REQUIRE(was_canceled == false);
    }

    SUBCASE("Cancellation before start is detected") {
        bool was_canceled = false;
        cancellation_point token;
        token.cancel();

        auto a0 = on(token) | [&](cancellation_point token, int x) {
            was_canceled = token.canceled();
            if (token.canceled()) return 0;
            return x * 2;
        };

        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 0);
        REQUIRE(was_canceled == true);
    }

    SUBCASE("Function can check cancellation and return early") {
        cancellation_point token;
        token.cancel();

        auto a0 = on(token) | [](cancellation_point token, int x) {
            if (token.canceled()) return -1;
            return x * 2;
        };

        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == -1);
    }

    SUBCASE("Cancellation token affects only first function in chain") {
        bool first_canceled = false;
        bool second_executed = false;
        cancellation_point token;
        token.cancel();

        auto a0 = on(token) | [&](cancellation_point token, int x) {
            first_canceled = token.canceled();
            if (token.canceled()) return 0;
            return x * 2;
        };
        auto a1 = std::move(a0) | on(immediate_executor) | [&](int x) {
            second_executed = true;
            return x + 10;
        };

        auto f = start(std::move(a1), 5);
        REQUIRE(f.get_ready() == 10);
        REQUIRE(first_canceled == true);
        REQUIRE(second_executed == true);
    }

    SUBCASE("Multiple functions can receive cancellation tokens") {
        bool first_canceled = false;
        bool second_canceled = false;
        cancellation_point token1;
        cancellation_point token2;
        token1.cancel();

        auto a0 = on(token1) |
                  [&](cancellation_point token, int x) {
                      first_canceled = token.canceled();
                      if (token.canceled()) return 0;
                      return x * 2;
                  } |
                  on(token2) | [&](cancellation_point token, int x) {
                      second_canceled = token.canceled();
                      return x + 10;
                  };

        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 10);
        REQUIRE(first_canceled == true);
        REQUIRE(second_canceled == false);
    }

    SUBCASE("Cancellation with string operations") {
        cancellation_point token;

        auto a0 = on(token) | [](cancellation_point token, const std::string& s) {
            if (token.canceled()) return std::string("canceled");
            return s + " processed";
        };

        auto f = start(std::move(a0), std::string("test"));
        REQUIRE(f.get_ready() == std::string("test processed"));
    }

    SUBCASE("Cancellation with string operations when canceled") {
        cancellation_point token;
        token.cancel();

        auto a0 = on(token) | [](cancellation_point token, const std::string& s) {
            if (token.canceled()) return std::string("canceled");
            return s + " processed";
        };

        auto f = start(std::move(a0), std::string("test"));
        REQUIRE(f.get_ready() == std::string("canceled"));
    }

    SUBCASE("Cancellation in complex pipeline") {
        cancellation_point token;
        bool was_canceled = false;

        auto a0 = on(immediate_executor) | [](int x) { return x + 5; } | on(token) |
                  [&](cancellation_point token, int x) {
                      was_canceled = token.canceled();
                      if (token.canceled()) return 0;
                      return x * 2;
                  } |
                  on(immediate_executor) | [](int x) { return x - 3; };

        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 27); // ((10+5)*2)-3
        REQUIRE(was_canceled == false);
    }

    SUBCASE("Cancellation in complex pipeline when canceled") {
        cancellation_point token;
        token.cancel();
        bool was_canceled = false;

        auto a0 = on(immediate_executor) | [](int x) { return x + 5; } | on(token) |
                  [&](cancellation_point token, int x) {
                      was_canceled = token.canceled();
                      if (token.canceled()) return 0;
                      return x * 2;
                  } |
                  on(immediate_executor) | [](int x) { return x - 3; };

        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == -3); // (0)-3
        REQUIRE(was_canceled == true);
    }

    SUBCASE("Cancellation with pair return type") {
        cancellation_point token;

        auto a0 = on(token) | [](cancellation_point token, int x) {
            if (token.canceled()) return std::make_pair(0, 0);
            return std::make_pair(x, x * 2);
        };

        auto f = start(std::move(a0), 5);
        auto result = f.get_ready();
        REQUIRE(result.first == 5);
        REQUIRE(result.second == 10);
    }

    SUBCASE("Cancellation with tuple operations") {
        cancellation_point token;

        auto a0 = on(token) | [](cancellation_point token, int x) {
            if (token.canceled()) return std::make_tuple(0, 0);
            return std::make_tuple(x + 1, x * 2);
        };

        auto f = start(std::move(a0), 5);
        auto result = f.get_ready();
        REQUIRE(std::get<0>(result) == 6);
        REQUIRE(std::get<1>(result) == 10);
    }

    SUBCASE("Cancellation with floating-point operations") {
        cancellation_point token;

        auto a0 = on(token) | [](cancellation_point token, double x) {
            if (token.canceled()) return 0.0;
            return x * 2.5;
        };

        auto f = start(std::move(a0), 4.0);
        REQUIRE(f.get_ready() == 10.0);
    }

    SUBCASE("Cancellation state persists across multiple checks") {
        cancellation_point token;
        token.cancel();
        int check_count = 0;

        auto a0 = on(token) | [&](cancellation_point token, int x) {
            if (token.canceled()) check_count++;
            if (token.canceled()) check_count++;
            if (token.canceled()) check_count++;
            return x;
        };

        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 5);
        REQUIRE(check_count == 3);
    }
}

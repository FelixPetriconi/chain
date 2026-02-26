#include <stlab/chain/chain.hpp>
#include <stlab/chain/on.hpp>
#include <stlab/chain/segment.hpp>
#include <stlab/chain/start.hpp>

#include <catch2/catch_test_macros.hpp>

#include <stlab/concurrency/immediate_executor.hpp>

#include <string>
#include <utility>

using namespace stlab;

// ============================================================================
// Basic Chain Operations Tests
// ============================================================================

TEST_CASE("Basic chain operations", "[chain]") {
    SECTION("Chain with single lambda") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; };
        auto f = start(std::move(a0), 42);
        REQUIRE(f.get_ready() == 84);
    }

    SECTION("Chain with two lambdas") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return std::to_string(x); } | on(immediate_executor) |
                  [](const std::string& s) { return s + "!"; };

        auto f = start(std::move(a0), 42);
        auto val = f.get_ready();
        REQUIRE(val == std::string("84!"));
    }

    SECTION("Chain with three lambdas") {
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

TEST_CASE("Type conversions in chain", "[chain][types]") {
    SECTION("int to string") {
        auto a0 = on(immediate_executor) | [](int x) { return std::to_string(x); };
        auto f = start(std::move(a0), 123);
        REQUIRE(f.get_ready() == std::string("123"));
    }

    SECTION("string to int") {
        auto a0 = on(immediate_executor) |
                  [](const std::string& s) { return static_cast<int>(s.length()); };
        auto f = start(std::move(a0), std::string("hello"));
        REQUIRE(f.get_ready() == 5);
    }

    SECTION("int to double") {
        auto a0 = on(immediate_executor) | [](int x) { return static_cast<double>(x) * 1.5; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 15.0);
    }

    SECTION("double to int") {
        auto a0 = on(immediate_executor) | [](double x) { return static_cast<int>(x); };
        auto f = start(std::move(a0), 3.7);
        REQUIRE(f.get_ready() == 3);
    }
}

// ============================================================================
// Arithmetic Operations Tests
// ============================================================================

TEST_CASE("Arithmetic operations in chain", "[chain][arithmetic]") {
    SECTION("Addition chain") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 5; } | on(immediate_executor) |
                  [](int x) { return x + 3; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 18);
    }

    SECTION("Multiplication chain") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x * 3; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 30);
    }

    SECTION("Mixed arithmetic operations") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x + 10; } | on(immediate_executor) |
                  [](int x) { return x / 2; };
        auto f = start(std::move(a0), 8);
        REQUIRE(f.get_ready() == 13); // ((8*2)+10)/2
    }

    SECTION("Modulo operations") {
        auto a0 = on(immediate_executor) | [](int x) { return x % 5; };
        auto f = start(std::move(a0), 17);
        REQUIRE(f.get_ready() == 2);
    }
}

// ============================================================================
// String Operations Tests
// ============================================================================

TEST_CASE("String operations in chain", "[chain][strings]") {
    SECTION("String concatenation") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s + " World"; };
        auto f = start(std::move(a0), std::string("Hello"));
        REQUIRE(f.get_ready() == std::string("Hello World"));
    }

    SECTION("String length operation") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s.length(); };
        auto f = start(std::move(a0), std::string("test"));
        REQUIRE(f.get_ready() == 4);
    }

    SECTION("Chain string transformations") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s + "!"; } |
                  on(immediate_executor) | [](const std::string& s) { return s + "!"; };
        auto f = start(std::move(a0), std::string("Hi"));
        REQUIRE(f.get_ready() == std::string("Hi!!"));
    }

    SECTION("String to uppercase pattern") {
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

TEST_CASE("Conditional logic in chain", "[chain][conditionals]") {
    SECTION("Simple conditional") {
        auto a0 = on(immediate_executor) | [](int x) { return x > 10 ? 100 : 50; };
        auto f = start(std::move(a0), 15);
        REQUIRE(f.get_ready() == 100);
    }

    SECTION("Conditional returning false") {
        auto a0 = on(immediate_executor) | [](int x) { return x > 10 ? 100 : 50; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 50);
    }

    SECTION("Chain with conditional transformation") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x > 50 ? true : false; };
        auto f = start(std::move(a0), 30);
        REQUIRE(f.get_ready() == true);
    }

    SECTION("Multiple conditional chains") {
        auto a0 = on(immediate_executor) | [](int x) { return x > 0 ? x : -x; } |
                  on(immediate_executor) | [](int x) { return x < 10 ? 10 : x; };
        auto f = start(std::move(a0), -5);
        REQUIRE(f.get_ready() == 10);
    }
}

// ============================================================================
// Edge Cases and Boundary Tests
// ============================================================================

TEST_CASE("Edge cases and boundary conditions", "[chain][edge-cases]") {
    SECTION("Zero as input") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 0; };
        auto f = start(std::move(a0), 0);
        REQUIRE(f.get_ready() == 0);
    }

    SECTION("Negative numbers") {
        auto a0 = on(immediate_executor) | [](int x) { return x * -1; };
        auto f = start(std::move(a0), -5);
        REQUIRE(f.get_ready() == 5);
    }

    SECTION("Large numbers") {
        auto a0 = on(immediate_executor) | [](long x) { return x + 1000000L; };
        auto f = start(std::move(a0), 9000000L);
        REQUIRE(f.get_ready() == 10000000L);
    }

    SECTION("Empty string") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s.empty(); };
        auto f = start(std::move(a0), std::string(""));
        REQUIRE(f.get_ready() == true);
    }

    SECTION("Single character string") {
        auto a0 = on(immediate_executor) | [](const std::string& s) { return s.length(); };
        auto f = start(std::move(a0), std::string("a"));
        REQUIRE(f.get_ready() == 1);
    }

    SECTION("Chain with identity function") {
        auto a0 = on(immediate_executor) | [](int x) { return x; };
        auto f = start(std::move(a0), 42);
        REQUIRE(f.get_ready() == 42);
    }
}

// ============================================================================
// Future-Related Tests
// ============================================================================

TEST_CASE("Future operations", "[chain][futures]") {
    SECTION("Future is ready after start") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.is_ready());
    }

    SECTION("get_ready returns correct value") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 5; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 15);
    }

    SECTION("Multiple chain invocations with different inputs") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; };

        auto f1 = start(std::move(a0), 5);
        REQUIRE(f1.get_ready() == 10);

        auto a1 = on(immediate_executor) | [](int x) { return x * 2; };
        auto f2 = start(std::move(a1), 10);
        REQUIRE(f2.get_ready() == 20);
    }
}

// ============================================================================
// Functional Programming Tests
// ============================================================================

TEST_CASE("Functional programming patterns", "[chain][functional]") {
    SECTION("Map-like operation") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x + 1; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 11); // (5*2)+1
    }

    SECTION("Filter-like operation via conditional") {
        auto a0 = on(immediate_executor) | [](int x) { return (x % 2 == 0) ? x : 0; };
        auto f = start(std::move(a0), 4);
        REQUIRE(f.get_ready() == 4);
    }

    SECTION("Filter-like operation removing odd") {
        auto a0 = on(immediate_executor) | [](int x) { return (x % 2 == 0) ? x : 0; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 0);
    }

    SECTION("Fold-like operation in chain") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 10; } | on(immediate_executor) |
                  [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x - 5; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 25); // ((5+10)*2)-5
    }
}

// ============================================================================
// Complex Type Tests
// ============================================================================

TEST_CASE("Complex types in chain", "[chain][complex-types]") {
    SECTION("Return pair") {
        auto a0 = on(immediate_executor) | [](int x) { return std::make_pair(x, x * 2); };
        auto f = start(std::move(a0), 5);
        auto result = f.get_ready();
        REQUIRE(result.first == 5);
        REQUIRE(result.second == 10);
    }

    SECTION("Chain with tuple-like operations") {
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

TEST_CASE("Cancellation functionality", "[chain][cancellation]") {
    SECTION("with_cancellation basic operation") {
        cancellation_source src;
        auto a0 = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 2;
        };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 10);
    }

    SECTION("with_cancellation before cancel") {
        cancellation_source src;
        auto a0 = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 2;
        };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 10);
    }

    SECTION("with_cancellation after cancel") {
        cancellation_source src;
        src.cancel();
        auto a0 = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 3;
        };
        auto f = start(std::move(a0), 7);
        REQUIRE(f.get_ready() == 0);
    }

    SECTION("with_cancellation then chain operations") {
        cancellation_source src;
        auto a0 = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 2;
        } | [](int y) { return y + 10; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 20); // (5*2)+10
    }

    SECTION("with_cancellation cancel before chain") {
        cancellation_source src;
        src.cancel();
        auto a0 = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 2;
        } | [](int y) { return y + 10; };
        auto f = start(std::move(a0), 5);
        REQUIRE(f.get_ready() == 10); // 0+10
    }
}

// ============================================================================
// Multi-Stage Pipeline Tests
// ============================================================================

TEST_CASE("Multi-stage pipelines", "[chain][pipelines]") {
    SECTION("Five-stage pipeline") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 1; } | on(immediate_executor) |
                  [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return x - 3; } | on(immediate_executor) |
                  [](int x) { return x / 2; } | on(immediate_executor) |
                  [](int x) { return x + 10; };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 14); // (((10+1)*2-3)/2)+10
    }

    SECTION("Type-changing pipeline") {
        auto a0 = on(immediate_executor) | [](int x) { return x + 5; } | on(immediate_executor) |
                  [](int x) { return std::to_string(x); } | on(immediate_executor) |
                  [](const std::string& s) { return s + "!"; } | on(immediate_executor) |
                  [](const std::string& s) { return s.length(); };
        auto f = start(std::move(a0), 10);
        REQUIRE(f.get_ready() == 4); // "15!" has length 4
    }
}

// ============================================================================
// Double/Float Operations Tests
// ============================================================================

TEST_CASE("Floating-point operations", "[chain][floats]") {
    SECTION("Simple double multiplication") {
        auto a0 = on(immediate_executor) | [](double x) { return x * 2.5; };
        auto f = start(std::move(a0), 4.0);
        REQUIRE(f.get_ready() == 10.0);
    }

    SECTION("Double precision chain") {
        auto a0 = on(immediate_executor) | [](double x) { return x + 0.1; } |
                  on(immediate_executor) | [](double x) { return x * 2.0; };
        auto f = start(std::move(a0), 1.0);
        REQUIRE(f.get_ready() == 2.2);
    }

    SECTION("Float to int conversion") {
        auto a0 = on(immediate_executor) | [](float x) { return x * 2.0f; } |
                  on(immediate_executor) | [](float x) { return static_cast<int>(x); };
        auto f = start(std::move(a0), 2.5f);
        REQUIRE(f.get_ready() == 5);
    }
}

#if 0

#include <iostream>
#include <stlab/concurrency/default_executor.hpp>
#include <stlab/test/model.hpp>
#include <thread>

using namespace std;
using namespace chain;
using namespace stlab;

// Cancellation example

TEST_CASE("Cancellation injection", "[initial_draft]") {
    {
        cancellation_source src;

        // Build a chain where the first function expects the token as first argument.
        auto c = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 2;
        } | [](int y) { return y + 10; }; // token only needed by first step

        auto f = start(std::move(c), 5);
        REQUIRE(f.get_ready() == 20); // (5*2)+10

        // Demonstrate cancel before start
        src.cancel();
        auto c2 = with_cancellation(src) | [](cancellation_token token, int x) {
            if (token.canceled()) return 0;
            return x * 3;
        };
        auto f2 = start(std::move(c2), 7);
        REQUIRE(f2.get_ready() == 0);
    }

    //{
    //    cancellation_source src;

    //    // Build a chain where each function expects the token as first argument.
    //    // First function uses the token, returns an int.
    //    auto c = with_cancellation(src) | [](cancellation_token token, int x) {
    //        if (token.canceled()) return 0;
    //        return x * 2;
    //    } | [](int y) { return y + 10; }; // token only needed by first step

    //    auto f = start(std::move(c), 5);
    //    REQUIRE(f.get_ready() == 20); // (5*2)+10
    //}
}

// --- Example test demonstrating split ---------------------------------------------------------
TEST_CASE("Split fan-out", "[initial_draft]") {
    auto base = on(immediate_executor) | [](int a) { return a; } | [](int x) { return x + 5; };
    auto splitter = split(std::move(base));
    auto left = splitter.fan([](int v) { return v * 2; }) | [](int x) { return x + 1; };
    auto right = splitter.fan([](int v) { return std::string("v=") + std::to_string(v); });

    auto f_right = start(std::move(right), 10);
    auto f_left = start(std::move(left), 5);
    REQUIRE(f_right.get_ready() == std::string("v=15"));
    REQUIRE(f_left.get_ready() == 31);
}

TEST_CASE("Split fan-out bound", "[initial_draft]") {
    auto base = on(immediate_executor) | [](int a) { return a; } | [](int x) { return x + 5; };

    // Bind upstream start argument 10 once:
    auto splitter = split_bind(std::move(base), 10);

    // Branches now start with no args; upstream result (15) is injected.
    auto left = splitter.fan([](int v) { return v * 2; }) | [](int x) { return x + 1; };
    auto right = splitter.fan([](int v) { return std::string("v=") + std::to_string(v); });

    auto f_right = start(std::move(right)); // no argument
    auto f_left = start(std::move(left));   // no argument

    REQUIRE(f_right.get_ready() == std::string("v=15"));
    REQUIRE(f_left.get_ready() == 31);
}

TEST_CASE("Initial draft", "[initial_draft]") {
    GIVEN("a sequence of callables with different arguments") {
        auto oneInt2Int = [](int a) { return a * 2; };
        auto twoInt2Int = [](int a, int b) { return a + b; };
        auto void2Int = []() { return 42; };

        auto a0 = on(stlab::immediate_executor) | oneInt2Int | void2Int | twoInt2Int;

        auto f = start(std::move(a0), 2);
        REQUIRE(f.is_ready());
        auto val = f.get_ready();
        REQUIRE(46 == val);
    }

    GIVEN("a sequence of callables that just work with move only value") {
        auto oneInt2Int = [](move_only a) { return move_only(a.member() * 2); };
        auto twoInt2Int = [](move_only a, move_only b) {
            return move_only(a.member() + b.member());
        };
        auto void2Int = []() { return move_only(42); };

        auto a0 = on(stlab::immediate_executor) | oneInt2Int | void2Int | twoInt2Int;

        auto f = start(std::move(a0), move_only(2));
        REQUIRE(f.is_ready());
        auto val = std::move(f).get_ready();
        REQUIRE(46 == val.member());
    }

    GIVEN("a sequence of callables in a chain of chain synchronous") {
        auto a0 = on(immediate_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return to_string(x); } | on(immediate_executor) |
                  [](const string& s) { return s + "!"; };

        auto f = start(std::move(a0), 42);
        auto val = f.get_ready();
        REQUIRE(val == string("84!"));
    }

    GIVEN("a sequence of callables in a chain of chain asynchronous") {
        auto a0 = on(default_executor) | [](int x) { return x * 2; } | on(immediate_executor) |
                  [](int x) { return to_string(x); } | on(default_executor) |
                  [](const string& s) { return s + "!"; };

        auto val = sync_wait(std::move(a0), 42);
        REQUIRE(val == string("84!"));
    }
}

TEST_CASE("Cancellation of then()", "[initial_draft]") {
    annotate_counters cnt;
    GIVEN("that a ") {
        auto fut =
            async(default_executor, [] {
                std::this_thread::sleep_for(std::chrono::seconds{3});
                std::cout << "Future did run" << std::endl;
                return std::string("42");
            }).then([_counter = annotate{cnt}](const auto& s) { std::cout << s << std::endl; });

        auto result_f = start(then(fut));
    }
    std::this_thread::sleep_for(std::chrono::seconds{5});
    std::cout << cnt << std::endl;
}
} // namespace chain

#endif

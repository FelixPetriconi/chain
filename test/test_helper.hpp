#ifndef STLAB_CHAIN_TEST_HELPER_HPP
#define STLAB_CHAIN_TEST_HELPER_HPP

#include <stlab/chain/config.hpp>
#include <stlab/test/model.hpp> // moveonly

#include <string>

namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE()::test_helper {
struct multi_callable {
    auto operator()(int a, float b) const -> int { return a * static_cast<int>(b); }
    auto operator()(int a, float b, int c) const -> int { return a + static_cast<int>(b) + c; }
};

struct intTimes2 {
    auto operator()(int a) const -> int { return a * 2; }
};
struct int2String {
    auto operator()(int a) const -> std::string { return std::to_string(a); }
};
struct oneInt2Int {
    auto operator()(int a) const -> int { return a; }
};

struct twoInt2Int {
    auto operator()(int a, int b) const -> int { return a + b; }
};
struct twoInt2void {
    int& hit;
    auto operator()(int a, int b) const -> void { hit = a + b; }
};
struct void2Int {
    auto operator()() const -> int { return 42; }
};
struct void2void {
    bool& hit;
    auto operator()() -> void { hit = true; }
};
struct oneInt2Void {
    int& hit;
    auto operator()(auto a) -> void { hit = a; }
};
struct string2Int {
    auto operator()(const std::string& s) const -> std::size_t { return s.size(); }
};
struct oneInt2String {
    auto operator()(int a) const -> std::string { return std::to_string(a); }
};
struct moveonly2Int {
    auto operator()(stlab::move_only m) const -> int { return m.member(); }
};
struct oneInt2Moveonly {
    auto operator()(int a) const -> stlab::move_only { return {a}; }
};
struct variadic2Int {
    auto operator()() const -> int { return 0; }
    auto operator()(auto... v) const -> int { return (v + ...); }
};
} // namespace stlab::inline STLAB_CHAIN_VERSION_NAMESPACE()::test_helper

#endif

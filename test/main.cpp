/*
    Copyright 2026 Adobe
    Distributed under the Boost Software License, Version 1.0.
    (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
*/

#define DOCTEST_CONFIG_IMPLEMENT
#include <doctest/doctest.h>

#include <stlab/pre_exit.hpp>

auto main(int argc, char** argv) -> int {
    doctest::Context ctx;
    ctx.applyCommandLine(argc, argv);
    int res = ctx.run();
    if (ctx.shouldExit()) return res;
    stlab::pre_exit();
    return res;
}

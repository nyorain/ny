// Copyright (c) 2017 nyorain
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt

#pragma once

#include <ny/fwd.hpp>
#include <dlg/dlg.hpp>

namespace ny {

using namespace dlg::literals;

#define ny_trace(...)  dlg_trace("ny"_project, __VA_ARGS__)
#define ny_debug(...)  dlg_debug("ny"_project, __VA_ARGS__)
#define ny_info(...) dlg_info("ny"_project, __VA_ARGS__)
#define ny_warn(...) dlg_warn("ny"_project, __VA_ARGS__)
#define ny_error(...) dlg_error("ny"_project, __VA_ARGS__)
#define ny_critical(...) dlg_critical("ny"_project, __VA_ARGS__)

#define ny_assert_debug(expr, ...) dlg_assert_debug(expr, "ny"_project, __VA_ARGS__)
#define ny_assert_warn(expr, ...) dlg_assert_warn(expr, "ny"_project, __VA_ARGS__)
#define ny_assert_error(expr, ...)  dlg_assert_error(expr, "ny"_project, __VA_ARGS__)
#define ny_assert_critical(expr, ...) dlg_assert_critical(expr, "ny"_project, __VA_ARGS__)

#define ny_log(...) dlg_log("ny"_project, __VA_ARGS__)
#define ny_assert(expr, ...) dlg_assert(expr, "ny"_project, __VA_ARGS__)

} // namespace ny

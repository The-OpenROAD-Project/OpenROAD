// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

namespace utl {
class Logger;
}
namespace sta {
class dbSta;
}
struct Tcl_Interp;

namespace cgt {

class ClockGating;

ClockGating* makeClockGating();

void initClockGating(ClockGating* cgt,
                     Tcl_Interp* tcl_interp,
                     utl::Logger* logger,
                     sta::dbSta* sta);

void deleteClockGating(ClockGating* clock_gating);

}  // namespace cgt

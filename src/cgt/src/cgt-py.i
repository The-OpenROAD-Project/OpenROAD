// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

%{
#include "cgt/ClockGating.h"

namespace ord {
// Defined in OpenRoad.i
cgt::ClockGating *
getClockGating();
}
%}

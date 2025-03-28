// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025, The OpenROAD Authors

#pragma once

namespace ord {
class OpenRoad;
}

namespace cgt {

class ClockGating;

ClockGating* makeClockGating();

void initClockGating(ord::OpenRoad* openroad);

void deleteClockGating(ClockGating* clock_gating);

}  // namespace cgt

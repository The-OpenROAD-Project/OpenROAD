// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "timing_report.h"
#include "web/core.h"

namespace web {

// Which parts of a path to draw: the Qt "Timing Path" display controls.
struct TimingPathParts
{
  bool data = true;
  bool launch_clock = true;
  bool capture_clock = true;
  bool legend = true;
};

// Draws `path` as TimingPathRenderer does (staGui.cpp): its instances,
// terminals and driver-to-sink nets, colored data, launch or capture clock.
void paintTimingPath(const TimingPathSummary& path,
                     const TimingPathParts& parts,
                     Painter& painter);

}  // namespace web

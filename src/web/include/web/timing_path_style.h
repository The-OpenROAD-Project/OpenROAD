// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include "web/core.h"

namespace web {

// TimingPathRenderer's display-control group and its rows, all on by default.
inline constexpr const char* kTimingPathGroup = "Timing Path";
inline constexpr const char* kTimingPathDataRow = "Data path";
inline constexpr const char* kTimingPathLaunchClockRow = "Launch clock";
inline constexpr const char* kTimingPathCaptureClockRow = "Capture clock";
inline constexpr const char* kTimingPathLegendRow = "Legend";

// How a timing path is drawn, by the Qt gui's TimingPathRenderer and by
// save_image -web alike.
inline constexpr Painter::Color kTimingPathInstColor(Painter::kMagenta, 100);
inline constexpr Painter::Color kTimingPathTermColor(Painter::kBlue, 100);
inline constexpr Painter::Color kTimingPathSignalColor(Painter::kRed, 100);
inline constexpr Painter::Color kTimingPathClockColor(Painter::kCyan, 100);
inline constexpr Painter::Color kTimingPathCaptureClockColor(Painter::kGreen,
                                                             100);

inline void paintTimingPathLegend(Painter& painter)
{
  DiscreteLegend legend;
  legend.addLegendKey(kTimingPathCaptureClockColor, "Capture");
  legend.addLegendKey(kTimingPathClockColor, "Launch");
  legend.addLegendKey(kTimingPathSignalColor, "Signal");
  legend.addLegendKey(kTimingPathInstColor, "Inst");
  legend.addLegendKey(kTimingPathTermColor, "Term");
  legend.draw(painter);
}

}  // namespace web

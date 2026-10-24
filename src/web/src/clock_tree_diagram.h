// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <optional>
#include <string>
#include <vector>

#include "web/clockTree.h"

namespace web {

class TileGenerator;

// ClockTreeView's scene (clockWidget.cpp) without Qt: where it puts each node
// and net, in its scene units, with y growing with the arrival time.
struct ClockTreeDiagram
{
  enum class Kind
  {
    kRoot,
    kBuffer,
    kInverter,
    kClockGate,
    // A cell that is neither a buffer nor a clock gate.
    kAssumedClockGate,
    kRegister,
    kMacro,
  };
  struct Point
  {
    double x = 0;
    double y = 0;
  };
  struct Node
  {
    Kind kind = Kind::kBuffer;
    // The input anchor; for the root, its output.
    Point pos{};
    double size = 0;
    // Buffers and gates: from the input to the output.  Macros: the insertion
    // delay.
    double delay_y = 0;
    bool visible = true;
    // The instance, or for a port root the port.
    std::string name{};
  };
  // A line, or a quadratic curve through `control`.
  struct Segment
  {
    Point from;
    Point to;
    std::optional<Point> control;
  };

  std::vector<Node> nodes;
  std::vector<std::vector<Segment>> nets;
  // In STA units, and the scene height they span.
  double min_delay = 0;
  double max_delay = 0;
  double time_scale = 0;
  // Of the time unit the scale is labeled in.
  double unit_scale = 1;
  std::string unit_suffix;
  // The scene rect, margins included.
  Point scene_min;
  Point scene_max;

  double delayToY(double delay) const;
  double yToDelay(double y) const;
};

ClockTreeDiagram layoutClockTreeDiagram(const ClockTree& tree);

// As ClockTreeView::save renders the view: the diagram fit to width x height
// on white, with the time scale down the left edge.  RGBA, top row first.
std::vector<unsigned char> renderClockTreeDiagram(
    const ClockTreeDiagram& diagram,
    int width,
    int height,
    const TileGenerator& tiles);

}  // namespace web

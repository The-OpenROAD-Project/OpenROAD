// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

namespace web {

struct Color
{
  unsigned char r = 0;  // Red (0-255)
  unsigned char g = 0;  // Green (0-255)
  unsigned char b = 0;  // Blue (0-255)
  unsigned char a = 0;  // Alpha (0-255)

  Color lighter(double factor = 1.5) const;
  Color darken(double factor = 0.5) const;
};

// Per-layer brush pattern used when rasterizing layer shapes.  The integer
// values mirror gui::Painter::Brush so the web frontend, this enum and the
// Qt GUI all agree on the ordering.
enum class FillPattern
{
  kNone = 0,      // no fill (skip)
  kSolid = 1,     // full fill (default; current behavior)
  kDiagonal = 2,  // diagonal hatch
  kCross = 3,     // crosshatch grid
  kDots = 4,      // dotted
};

}  // namespace web

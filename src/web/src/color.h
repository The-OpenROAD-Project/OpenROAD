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

// Selection-highlight yellow shared by the tile highlight border
// (TileGenerator::drawHighlight) and the selection flywires
// (request_handler's collectNetFlightLines).
inline constexpr Color kSelectionYellow{.r = 255, .g = 255, .b = 0, .a = 255};

}  // namespace web

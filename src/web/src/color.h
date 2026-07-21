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

// Number of entries in the built-in spectrum (Turbo) colormap.
inline constexpr int kSpectrumColorCount = 256;

// Map a normalized value in [0, 1] to a color on the Turbo colormap, with the
// given alpha.  Values outside [0, 1] are clamped.  This is a self-contained
// port of gui::SpectrumGenerator's 256-entry table so libweb has no link
// dependency on the Qt GUI.  Used by the timing-cone overlay to color pins and
// flight lines by slack (or logic depth).
Color spectrumColor(double value, unsigned char alpha = 255);

}  // namespace web

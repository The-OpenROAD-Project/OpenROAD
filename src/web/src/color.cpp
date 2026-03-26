// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "color.h"

#include <algorithm>  // For std::min, std::max
#include <cmath>

namespace web {

struct HSL
{
  double h;  // Hue (0-360)
  double s;  // Saturation (0-1)
  double l;  // Lightness (0-1)
};

// Converts your existing Color struct (RGB) to HSL
static HSL rgb_to_hsl(const Color& rgb)
{
  // Normalize R, G, B to 0-1
  double r = rgb.r / 255.0;
  double g = rgb.g / 255.0;
  double b = rgb.b / 255.0;

  double vmin = std::min({r, g, b});
  double vmax = std::max({r, g, b});
  double delta = vmax - vmin;

  HSL hsl;
  hsl.l = (vmax + vmin) / 2.0;  // Lightness

  if (delta == 0.0) {
    hsl.h = 0.0;  // Achromatic (grey)
    hsl.s = 0.0;  // Saturation
  } else {
    hsl.s = (hsl.l < 0.5) ? (delta / (vmax + vmin))
                          : (delta / (2.0 - vmax - vmin));  // Saturation

    // Hue
    if (vmax == r) {
      hsl.h = 60.0 * (g - b) / delta;
    } else if (vmax == g) {
      hsl.h = 60.0 * (2.0 + (b - r) / delta);
    } else {  // vmax == b
      hsl.h = 60.0 * (4.0 + (r - g) / delta);
    }

    if (hsl.h < 0.0) {
      hsl.h += 360.0;  // Ensure hue is positive
    }
  }
  return hsl;
}

// Helper for HSL to RGB conversion
static double hue_to_rgb(double p, double q, double t)
{
  if (t < 0.0) {
    t += 1.0;
  }
  if (t > 1.0) {
    t -= 1.0;
  }
  if (t < 1.0 / 6.0) {
    return p + (q - p) * 6.0 * t;
  }
  if (t < 1.0 / 2.0) {
    return q;
  }
  if (t < 2.0 / 3.0) {
    return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
  }
  return p;
}

// Converts an HSL struct back to your Color struct (RGB)
static Color hsl_to_rgb(const HSL& hsl, const unsigned char a)
{
  Color rgb;
  rgb.a = a;

  if (hsl.s == 0.0) {
    // Achromatic (grey)
    unsigned char l = (unsigned char) std::round(hsl.l * 255.0);
    rgb.r = l;
    rgb.g = l;
    rgb.b = l;
  } else {
    double q = (hsl.l < 0.5) ? (hsl.l * (1.0 + hsl.s))
                             : (hsl.l + hsl.s - hsl.l * hsl.s);
    double p = 2.0 * hsl.l - q;
    double h_norm = hsl.h / 360.0;

    rgb.r = (unsigned char) std::round(hue_to_rgb(p, q, h_norm + 1.0 / 3.0)
                                       * 255.0);
    rgb.g = (unsigned char) std::round(hue_to_rgb(p, q, h_norm) * 255.0);
    rgb.b = (unsigned char) std::round(hue_to_rgb(p, q, h_norm - 1.0 / 3.0)
                                       * 255.0);
  }
  return rgb;
}

// factor is a value > 1.0. 1.5 is a 50% increase in lightness.
Color Color::lighter(double factor) const
{
  HSL hsl = rgb_to_hsl(*this);
  hsl.l = std::min(1.0, hsl.l * factor);
  return hsl_to_rgb(hsl, a);
}

// Factor is 0.0 - 1.0, e.g., 0.8 is 20% darker
Color Color::darken(double factor) const
{
  HSL hsl = rgb_to_hsl(*this);
  hsl.l = std::max(0.0, hsl.l * factor);
  return hsl_to_rgb(hsl, a);
}

}  // namespace web

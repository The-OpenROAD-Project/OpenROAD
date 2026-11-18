// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#include "colorGenerator.h"

#include <array>

#include "web/core.h"

namespace web {

using Color = Painter::Color;

// https://mokole.com/palette.html
const std::array<Color, 31> ColorGenerator::kColors{
    Color{255, 0, 0},     Color{255, 140, 0},   Color{255, 215, 0},
    Color{0, 255, 0},     Color{148, 0, 211},   Color{0, 250, 154},
    Color{220, 20, 60},   Color{0, 255, 255},   Color{0, 191, 255},
    Color{0, 0, 255},     Color{173, 255, 47},  Color{218, 112, 214},
    Color{255, 0, 255},   Color{30, 144, 255},  Color{250, 128, 114},
    Color{176, 224, 230}, Color{255, 20, 147},  Color{123, 104, 238},
    Color{255, 250, 205}, Color{255, 182, 193}, Color{85, 107, 47},
    Color{139, 69, 19},   Color{72, 61, 139},   Color{0, 128, 0},
    Color{60, 179, 113},  Color{184, 134, 11},  Color{0, 139, 139},
    Color{0, 0, 139},     Color{50, 205, 50},   Color{128, 0, 128},
    Color{176, 48, 96}};

ColorGenerator::ColorGenerator() : index_(0)
{
}

Painter::Color ColorGenerator::getColor()
{
  const Color color = kColors[index_++];
  if (index_ == getColorCount()) {
    index_ = 0;
  }
  return color;
}

}  // namespace web

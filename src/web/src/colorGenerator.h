// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <array>

#include "web/core.h"

namespace web {

class ColorGenerator
{
 public:
  ColorGenerator();

  int getColorCount() const { return kColors.size(); }
  Painter::Color getColor();

  void reset() { index_ = 0; }

 private:
  static const std::array<Painter::Color, 31> kColors;
  int index_;
};

}  // namespace web

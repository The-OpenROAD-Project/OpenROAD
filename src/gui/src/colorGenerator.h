// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <QColor>
#include <array>

#include "gui/gui.h"

namespace gui {

class ColorGenerator
{
 public:
  ColorGenerator();
  ~ColorGenerator() {}

  int getColorCount() const { return colors_.size(); }
  QColor getQColor();
  Painter::Color getColor();

  void reset() { index_ = 0; }

 private:
  static const std::array<QColor, 31> colors_;
  int index_;
};

}  // namespace gui

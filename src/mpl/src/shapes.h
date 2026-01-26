// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

#include <cstdint>
namespace mpl {

struct Interval
{
  Interval() = default;
  Interval(int min, int max) : min(min), max(max) {}

  int min{0};
  int max{0};
};

class Tiling
{
 public:
  Tiling() = default;
  Tiling(int width, int height) : width_(width), height_(height) {}

  int width() const { return width_; }
  int height() const { return height_; }
  int64_t area() const { return width_ * static_cast<int64_t>(height_); }
  float aspectRatio() const { return height_ / static_cast<float>(width_); }

  bool operator==(const Tiling& tiling) const;
  bool operator<(const Tiling& tiling) const;

 private:
  int width_{0};
  int height_{0};
};

}  // namespace mpl

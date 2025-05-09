// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2025-2025, The OpenROAD Authors

#pragma once

namespace mpl {

// Bounds of a certain cluster's dimension.
// Used for either width or height.
struct Interval
{
  Interval(float min, float max) : min(min), max(max) {}

  float min{0.0f};
  float max{0.0f};
};

// Coarse shape of a cluster that contains macros.
class Tiling
{
 public:
  Tiling() = default;
  Tiling(float width, float height) : width_(width), height_(height) {}

  float width() const { return width_; }
  float height() const { return height_; }
  float area() const { return width_ * height_; }
  float aspectRatio() const { return height_ / width_; }

  bool operator==(const Tiling& tiling) const;
  bool operator<(const Tiling& tiling) const;

 private:
  float width_{0.0f};
  float height_{0.0f};
};

}  // namespace mpl

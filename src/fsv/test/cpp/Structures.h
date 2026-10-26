// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// The structures of test/golden, for the golden tests and the benchmark.

#pragma once

#include <string>
#include <string_view>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace fsv {

// The directory of the structures, relative to the runfiles root.
inline constexpr std::string_view kGoldenDir = "src/fsv/test/golden/";

struct Structure
{
  Geometry geometry;
  Stack stack;
};

// Reads the golden structure format: "background <eps>", "layer <top>
// <eps>" bottom-up from the ground plane, "box <net> x0 y0 z0 x1 y1 z1", and
// "conformal <name> <bottom> <top> <SW_T> <TW_T> <eps>", a conformal
// dielectric of the conductor layer [bottom, top] (nesting outward in
// order). Cross sections run along y, ignoring the boxes' y extents; a
// positive `length` replaces every box's y extent with [0, length]. Throws
// std::runtime_error if the file cannot be read.
Structure readStructure(const std::string& name, double length = 0);

// A cross-section structure's rectangles: its boxes' x and z extents.
Geometry2D crossSection(const Geometry& geometry);

}  // namespace fsv

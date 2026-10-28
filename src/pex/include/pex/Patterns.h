// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

// Builds fsv geometries from a process stack by layer handle: the patterns
// behind the model tables and the experiments.

#pragma once

#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace pex {

using Layer = fsv::Stack::Layer;

// The z extent of a conductor or via layer. Throws std::invalid_argument
// for a handle that names no layer, or a via that lands on the ground
// plane.
fsv::Stack::ZExtent layerExtent(const fsv::Stack& stack, Layer layer);

// The conductor layers below (or above) a conductor, nearest first, by
// their z extents: a layer counts as below when its top is at or under the
// layer's bottom.
std::vector<Layer> layersBelow(const fsv::Stack& stack, Layer layer);
std::vector<Layer> layersAbove(const fsv::Stack& stack, Layer layer);

// A net in a fsv::Geometry or fsv::Geometry2D, numbered as addNet does.
using NetIndex = int;
// No net: a pattern element that was not built.
constexpr NetIndex kNullNetIndex = -1;

// A 3D geometry whose boxes take their z extent from a layer.
class PatternBuilder
{
 public:
  explicit PatternBuilder(const fsv::Stack& stack) : stack_(stack) {}
  // Continues a geometry, such as one a pattern built.
  PatternBuilder(const fsv::Stack& stack, fsv::Geometry geometry)
      : stack_(stack), geometry_(std::move(geometry))
  {
  }

  // The index of a net, added on first use.
  NetIndex net(std::string_view name);
  // Adds a box of `layer` over [x0, x1] x [y0, y1] to a net.
  void add(Layer layer,
           NetIndex net,
           double x0,
           double y0,
           double x1,
           double y1);

  const fsv::Stack& stack() const { return stack_; }
  fsv::Geometry& geometry() { return geometry_; }
  const fsv::Geometry& geometry() const { return geometry_; }

 private:
  const fsv::Stack& stack_;
  fsv::Geometry geometry_;
};

// A cross section whose rectangles take their z extent from a layer; the
// wires run along y.
class SectionBuilder
{
 public:
  explicit SectionBuilder(const fsv::Stack& stack) : stack_(stack) {}

  NetIndex net(std::string_view name);
  // Adds a rectangle of `layer` over [x0, x1] to a net.
  void add(Layer layer, NetIndex net, double x0, double x1);

  const fsv::Stack& stack() const { return stack_; }
  fsv::Geometry2D& geometry() { return geometry_; }
  const fsv::Geometry2D& geometry() const { return geometry_; }

 private:
  const fsv::Stack& stack_;
  fsv::Geometry2D geometry_;
};

// The cross section behind the lateral table: a wire centered at x = 0, a
// neighbor on each side at a spacing (none when negative), and a plane on
// the covering layer below and above (none when the handle names no
// layer), reaching `extent` past the outermost wire on each side.
struct LateralPattern
{
  Layer layer;
  double width;
  double left = -1;           // spacing to the left neighbor, µm
  double right = -1;          // spacing to the right neighbor, µm
  double neighbor_width = 0;  // 0: the layer's minimum width
  Layer below;                // none: the substrate alone
  Layer above;                // none: open above
  double extent = 10;         // the planes' reach past the wires, µm

  // The geometry, with the net index of each part (-1 when absent).
  struct Section
  {
    fsv::Geometry2D geometry;
    NetIndex wire = kNullNetIndex;
    NetIndex left = kNullNetIndex;
    NetIndex right = kNullNetIndex;
    NetIndex below = kNullNetIndex;
    NetIndex above = kNullNetIndex;
  };
  Section build(const fsv::Stack& stack) const;
};

// The pattern behind the end table: a wire along y over [0, length] with
// a plane on the covering layer below and above (none when the handle
// names no layer), reaching `extent` sideways and `margin` past both ends.
struct EndPattern
{
  Layer layer;
  double width;
  double length;
  Layer below;  // none: the substrate alone
  Layer above;  // none: open above
  double extent = 10;
  double margin = 2;

  struct Wire
  {
    fsv::Geometry geometry;
    NetIndex wire = kNullNetIndex;
    NetIndex below = kNullNetIndex;
    NetIndex above = kNullNetIndex;
  };
  Wire build(const fsv::Stack& stack) const;
};

// The pattern behind the crossing table: a wire along y over [0, length]
// under or over an array of crossing wires on `cover`, of a width and
// pitch, running `margin` past both ends alike. The length is rounded to
// whole pitches, at least one. Optionally neighbors at a spacing on both
// sides of the wire over the same span, and a plane on `other`, the layer
// on the other side of the wire, reaching `extent` sideways.
struct CrossingPattern
{
  Layer layer;
  Layer cover;
  double width;
  double crossing_width;
  double pitch;
  double length;
  double spacing = -1;  // to the neighbors; none when negative
  Layer other;          // none when the handle names no layer
  double extent = 10;
  double margin = 2;

  struct Array
  {
    fsv::Geometry geometry;
    double length = 0;  // the rounded length, µm
    NetIndex wire = kNullNetIndex;
    NetIndex cross = kNullNetIndex;
    NetIndex left = kNullNetIndex;
    NetIndex right = kNullNetIndex;
    NetIndex other = kNullNetIndex;
  };
  Array build(const fsv::Stack& stack) const;
};

// The cross section behind the diagonal table: a wire centered at x = 0,
// a neighbor at `spacing` on the right and another at `far_spacing` on the
// left (none when negative), and a parallel wire on `other` to the right,
// its left edge `gap` past the wire's right edge (overlapping when
// negative), of `other_width` (none when 0).
struct DiagonalPattern
{
  Layer layer;
  Layer other;
  double width;
  double other_width = 0;
  double gap = 0;
  double spacing = -1;
  double far_spacing = -1;

  struct Section
  {
    fsv::Geometry2D geometry;
    NetIndex wire = kNullNetIndex;
    NetIndex left = kNullNetIndex;
    NetIndex right = kNullNetIndex;
    NetIndex diagonal = kNullNetIndex;
  };
  Section build(const fsv::Stack& stack) const;
};

}  // namespace pex

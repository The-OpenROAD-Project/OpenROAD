// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "pex/Patterns.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "fsv/Geometry.h"
#include "fsv/Stack.h"

namespace pex {

fsv::Stack::ZExtent layerExtent(const fsv::Stack& stack, const Layer layer)
{
  if (!layer.valid()) {
    throw std::invalid_argument("no layer given");
  }
  if (stack.isVia(layer) && !stack.via(layer).lower.valid()) {
    throw std::invalid_argument("via " + stack.name(layer)
                                + " lands on the ground plane");
  }
  return stack.zExtent(layer);
}

namespace {

std::vector<Layer> layersBeside(const fsv::Stack& stack,
                                const Layer layer,
                                const bool below)
{
  const fsv::Stack::Conductor& c = stack.conductor(layer);
  std::vector<Layer> others;
  for (const Layer other : stack.conductorLayers()) {
    const fsv::Stack::Conductor& o = stack.conductor(other);
    if (other != layer && (below ? o.top <= c.bottom : o.bottom >= c.top)) {
      others.push_back(other);
    }
  }
  std::stable_sort(
      others.begin(), others.end(), [&](const Layer a, const Layer b) {
        const fsv::Stack::Conductor& ca = stack.conductor(a);
        const fsv::Stack::Conductor& cb = stack.conductor(b);
        return below ? ca.top > cb.top : ca.bottom < cb.bottom;
      });
  return others;
}

template <class Geometry>
int findOrAddNet(Geometry& geometry, const std::string_view name)
{
  for (NetIndex net = 0; net < geometry.netCount(); net++) {
    if (geometry.netName(net) == name) {
      return net;
    }
  }
  return geometry.addNet(name);
}

}  // namespace

std::vector<Layer> layersBelow(const fsv::Stack& stack, const Layer layer)
{
  return layersBeside(stack, layer, true);
}

std::vector<Layer> layersAbove(const fsv::Stack& stack, const Layer layer)
{
  return layersBeside(stack, layer, false);
}

double coverDistance(const fsv::Stack& stack,
                     const Layer layer,
                     const Layer cover)
{
  const fsv::Stack::Conductor& wire = stack.conductor(layer);
  const fsv::Stack::Conductor& c = stack.conductor(cover);
  return std::max(c.bottom - wire.top, wire.bottom - c.top);
}

double logCoverDistance(const double distance)
{
  return std::log(std::max(distance, kMinCoverDistance));
}

NetIndex PatternBuilder::net(const std::string_view name)
{
  return findOrAddNet(geometry_, name);
}

void PatternBuilder::add(const Layer layer,
                         const NetIndex net,
                         const double x0,
                         const double y0,
                         const double x1,
                         const double y1)
{
  const fsv::Stack::ZExtent z = layerExtent(stack_, layer);
  geometry_.addBox(net, {{x0, y0, z.bottom}, {x1, y1, z.top}});
}

NetIndex SectionBuilder::net(const std::string_view name)
{
  return findOrAddNet(geometry_, name);
}

void SectionBuilder::add(const Layer layer,
                         const NetIndex net,
                         const double x0,
                         const double x1)
{
  const fsv::Stack::ZExtent z = layerExtent(stack_, layer);
  geometry_.addRect(net, {{x0, z.bottom}, {x1, z.top}});
}

LateralPattern::Section LateralPattern::build(const fsv::Stack& stack) const
{
  const fsv::Stack::Conductor& conductor = stack.conductor(layer);
  if (width <= 0) {
    throw std::invalid_argument("the wire needs a positive width");
  }
  const double neighbor_w
      = neighbor_width > 0 ? neighbor_width : conductor.min_width;
  if (neighbor_w <= 0) {
    throw std::invalid_argument("the neighbors need a positive width");
  }
  SectionBuilder builder(stack);
  Section section;
  const double half = width / 2;
  double outer_left = -half;
  double outer_right = half;
  section.wire = builder.net("wire");
  builder.add(layer, section.wire, -half, half);
  if (left >= 0) {
    section.left = builder.net("left");
    const double x1 = -half - left;
    builder.add(layer, section.left, x1 - neighbor_w, x1);
    outer_left = x1 - neighbor_w;
  }
  if (right >= 0) {
    section.right = builder.net("right");
    const double x0 = half + right;
    builder.add(layer, section.right, x0, x0 + neighbor_w);
    outer_right = x0 + neighbor_w;
  }
  const double plane_left = outer_left - extent;
  const double plane_right = outer_right + extent;
  if (below.valid()) {
    section.below = builder.net("below");
    builder.add(below, section.below, plane_left, plane_right);
  }
  if (above.valid()) {
    section.above = builder.net("above");
    builder.add(above, section.above, plane_left, plane_right);
  }
  section.geometry = std::move(builder.geometry());
  return section;
}

EndPattern::Wire EndPattern::build(const fsv::Stack& stack) const
{
  if (width <= 0 || length <= 0) {
    throw std::invalid_argument("the wire needs a positive width and length");
  }
  PatternBuilder builder(stack);
  Wire w;
  w.wire = builder.net("wire");
  builder.add(layer, w.wire, -width / 2, 0, width / 2, length);
  double y0 = 0;
  double y1 = length;
  if (facing_gap >= 0) {
    w.facing = builder.net("facing");
    y0 = -facing_gap - length;
    y1 = 2 * length + facing_gap;
    builder.add(layer, w.facing, -width / 2, y0, width / 2, -facing_gap);
    builder.add(
        layer, w.facing, -width / 2, length + facing_gap, width / 2, y1);
  }
  if (left_spacing >= 0 || right_spacing >= 0) {
    const double nw = stack.conductor(layer).min_width;
    if (nw <= 0) {
      throw std::invalid_argument("the neighbors need a positive width");
    }
    if (left_spacing >= 0) {
      w.left = builder.net("left");
      const double x = width / 2 + left_spacing;
      builder.add(layer, w.left, -x - nw, y0 - margin, -x, y1 + margin);
    }
    if (right_spacing >= 0) {
      w.right = builder.net("right");
      const double x = width / 2 + right_spacing;
      builder.add(layer, w.right, x, y0 - margin, x + nw, y1 + margin);
    }
  }
  if (below.valid()) {
    w.below = builder.net("below");
    builder.add(below, w.below, -extent, y0 - margin, extent, y1 + margin);
  }
  if (above.valid()) {
    w.above = builder.net("above");
    builder.add(above, w.above, -extent, y0 - margin, extent, y1 + margin);
  }
  w.geometry = std::move(builder.geometry());
  return w;
}

CornerPattern::Wire CornerPattern::build(const fsv::Stack& stack) const
{
  if (width <= 0 || length <= width) {
    throw std::invalid_argument(
        "the corner pattern needs a positive width and a longer length");
  }
  PatternBuilder builder(stack);
  Wire w;
  w.wire = builder.net("wire");
  const double h = width / 2;
  builder.add(layer, w.wire, -h, 0, h, length);
  if (shape == Shape::kCross) {
    builder.add(layer,
                w.wire,
                -length / 2,
                (length / 2) - h,
                length / 2,
                (length / 2) + h);
  } else if (shape == Shape::kBend) {
    builder.add(layer, w.wire, -h, length - width, length - h, length);
  }
  const double r = length + extent;
  if (below.valid()) {
    w.below = builder.net("below");
    builder.add(below, w.below, -r, -r, r, r);
  }
  if (above.valid()) {
    w.above = builder.net("above");
    builder.add(above, w.above, -r, -r, r, r);
  }
  w.geometry = std::move(builder.geometry());
  return w;
}

CrossingPattern::Array CrossingPattern::build(const fsv::Stack& stack) const
{
  if (width <= 0 || crossing_width <= 0 || pitch <= 0 || length <= 0) {
    throw std::invalid_argument(
        "the crossing pattern needs positive width, pitch and length");
  }
  if (crossing_width > pitch) {
    throw std::invalid_argument("the crossings are wider than their pitch");
  }
  PatternBuilder builder(stack);
  Array array;
  const int n = std::max(1, static_cast<int>(std::lround(length / pitch)));
  array.length = n * pitch;
  const double w = width;
  const double y0 = -margin;
  const double y1 = array.length + margin;
  array.wire = builder.net("wire");
  builder.add(layer, array.wire, -w / 2, 0, w / 2, array.length);
  array.cross = builder.net("cross");
  const int extra = static_cast<int>(std::ceil(margin / pitch));
  // Each crossing is centered in its pitch cell, so the wire's ends at 0
  // and at n pitches both fall in the middle of a gap: the two lengths
  // then have the same end surroundings, and the coverage is exact.
  for (int k = -extra; k < n + extra; k++) {
    const double y = (k + 0.5) * pitch - crossing_width / 2;
    builder.add(cover, array.cross, -extent, y, extent, y + crossing_width);
  }
  if (spacing >= 0) {
    const double s = spacing;
    array.left = builder.net("left");
    builder.add(layer, array.left, -w / 2 - s - w, y0, -w / 2 - s, y1);
    array.right = builder.net("right");
    builder.add(layer, array.right, w / 2 + s, y0, w / 2 + s + w, y1);
  }
  if (other.valid()) {
    array.other = builder.net("other");
    builder.add(other, array.other, -extent, y0, extent, y1);
  }
  array.geometry = std::move(builder.geometry());
  return array;
}

ReachPattern::Pair ReachPattern::build(const fsv::Stack& stack) const
{
  if (width <= 0 || crossing_width <= 0 || past < 0) {
    throw std::invalid_argument(
        "the reach pattern needs positive widths and a past of at least 0");
  }
  PatternBuilder builder(stack);
  Pair pair;
  const double w = width;
  const double half = (crossing_width / 2) + past;
  pair.wire = builder.net("wire");
  builder.add(layer, pair.wire, -w / 2, -half, w / 2, half);
  if (extent >= 0) {
    pair.cross = builder.net("cross");
    builder.add(cover,
                pair.cross,
                -w / 2 - extent,
                -crossing_width / 2,
                w / 2 + extent,
                crossing_width / 2);
  }
  pair.geometry = std::move(builder.geometry());
  return pair;
}

DiagonalPattern::Section DiagonalPattern::build(const fsv::Stack& stack) const
{
  if (width <= 0) {
    throw std::invalid_argument("the wire needs a positive width");
  }
  SectionBuilder builder(stack);
  Section section;
  const double w = width;
  section.wire = builder.net("wire");
  builder.add(layer, section.wire, -w / 2, w / 2);
  if (far_spacing >= 0) {
    const double far = far_spacing;
    section.left = builder.net("left");
    builder.add(layer, section.left, -w / 2 - far - w, -w / 2 - far);
  }
  if (spacing >= 0) {
    const double s = spacing;
    section.right = builder.net("right");
    builder.add(layer, section.right, w / 2 + s, w / 2 + s + w);
  }
  if (other_width > 0) {
    section.diagonal = builder.net("diagonal");
    const double x0 = w / 2 + gap;
    builder.add(other, section.diagonal, x0, x0 + other_width);
  }
  if (opposite.valid()) {
    section.opposite = builder.net("opposite");
    builder.add(opposite,
                section.opposite,
                -w / 2 - extent,
                w / 2 + std::max(gap, 0.0) + other_width + extent);
  }
  section.geometry = std::move(builder.geometry());
  return section;
}

}  // namespace pex

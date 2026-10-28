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

ZRange zRange(const fsv::Stack& stack, const std::string_view layer)
{
  const fsv::Stack::ZExtent z = stack.zExtent(layer);
  switch (z.status) {
    case fsv::Stack::ZExtent::kFound:
      return {z.bottom, z.top};
    case fsv::Stack::ZExtent::kUnknownLayer:
      throw std::invalid_argument("the stack has no layer "
                                  + std::string(layer));
    case fsv::Stack::ZExtent::kViaToGround:
      throw std::invalid_argument("via " + std::string(layer)
                                  + " lands on the ground plane");
    case fsv::Stack::ZExtent::kViaWithoutUpper:
      throw std::invalid_argument("via " + std::string(layer)
                                  + " has no upper conductor");
  }
  throw std::invalid_argument("bad layer " + std::string(layer));
}

namespace {

std::vector<std::string> layersBeside(const fsv::Stack& stack,
                                      const std::string_view layer,
                                      const bool below)
{
  const ZRange z = zRange(stack, layer);
  std::vector<const fsv::Stack::Conductor*> others;
  for (const fsv::Stack::Conductor& c : stack.conductors) {
    if (c.name == layer) {
      continue;
    }
    if (below ? c.top <= z.bottom : c.bottom >= z.top) {
      others.push_back(&c);
    }
  }
  std::stable_sort(
      others.begin(),
      others.end(),
      [&](const fsv::Stack::Conductor* a, const fsv::Stack::Conductor* b) {
        return below ? a->top > b->top : a->bottom < b->bottom;
      });
  std::vector<std::string> names;
  names.reserve(others.size());
  for (const fsv::Stack::Conductor* c : others) {
    names.push_back(c->name);
  }
  return names;
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

std::vector<std::string> layersBelow(const fsv::Stack& stack,
                                     const std::string_view layer)
{
  return layersBeside(stack, layer, true);
}

std::vector<std::string> layersAbove(const fsv::Stack& stack,
                                     const std::string_view layer)
{
  return layersBeside(stack, layer, false);
}

NetIndex PatternBuilder::net(const std::string_view name)
{
  return findOrAddNet(geometry_, name);
}

void PatternBuilder::add(const std::string_view layer,
                         const NetIndex net,
                         const double x0,
                         const double y0,
                         const double x1,
                         const double y1)
{
  const ZRange z = zRange(stack_, layer);
  geometry_.addBox(net, {{x0, y0, z.bottom}, {x1, y1, z.top}});
}

NetIndex SectionBuilder::net(const std::string_view name)
{
  return findOrAddNet(geometry_, name);
}

void SectionBuilder::add(const std::string_view layer,
                         const NetIndex net,
                         const double x0,
                         const double x1)
{
  const ZRange z = zRange(stack_, layer);
  geometry_.addRect(net, {{x0, z.bottom}, {x1, z.top}});
}

LateralPattern::Section LateralPattern::build(const fsv::Stack& stack) const
{
  const fsv::Stack::Conductor* conductor = stack.findConductor(layer);
  if (conductor == nullptr) {
    throw std::invalid_argument("the stack has no conductor " + layer);
  }
  if (width <= 0) {
    throw std::invalid_argument("the wire needs a positive width");
  }
  const double neighbor_w
      = neighbor_width > 0 ? neighbor_width : conductor->min_width;
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
  if (!below.empty()) {
    section.below = builder.net("below");
    builder.add(below, section.below, plane_left, plane_right);
  }
  if (!above.empty()) {
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
  if (!below.empty()) {
    w.below = builder.net("below");
    builder.add(below, w.below, -extent, -margin, extent, length + margin);
  }
  if (!above.empty()) {
    w.above = builder.net("above");
    builder.add(above, w.above, -extent, -margin, extent, length + margin);
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
  if (!other.empty()) {
    array.other = builder.net("other");
    builder.add(other, array.other, -extent, y0, extent, y1);
  }
  array.geometry = std::move(builder.geometry());
  return array;
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
  section.geometry = std::move(builder.geometry());
  return section;
}

}  // namespace pex

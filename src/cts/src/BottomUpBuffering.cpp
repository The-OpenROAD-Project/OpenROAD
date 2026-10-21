// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "BottomUpBuffering.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace cts {
namespace {
double distance(BufferingPoint a, BufferingPoint b)
{
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

BufferingPoint midpoint(BufferingPoint a, BufferingPoint b)
{
  // Stay on a Manhattan path, rather than shortening a diagonal route.
  const double half = distance(a, b) / 2;
  const double dx = std::min(half, std::abs(a.x - b.x));
  a.x += std::copysign(dx, b.x - a.x);
  a.y += std::copysign(half - dx, b.y - a.y);
  return a;
}
}  // namespace

BufferingWireRC conservativeBufferingRC(std::span<const BufferingWireRC> rc)
{
  if (rc.empty()) {
    throw std::invalid_argument("No RC scenes available for bottom-up CTS");
  }
  BufferingWireRC bound;
  for (const auto& value : rc) {
    if (!std::isfinite(value.resistance) || value.resistance <= 0
        || !std::isfinite(value.capacitance) || value.capacitance <= 0) {
      throw std::invalid_argument(
          "Bottom-up CTS requires positive finite wire RC");
    }
    bound.resistance = std::max(bound.resistance, value.resistance);
    bound.capacitance = std::max(bound.capacitance, value.capacitance);
  }
  return bound;
}

BottomUpBuffering::BottomUpBuffering(double cap_per_dbu,
                                     double max_wire_length,
                                     Selector selector,
                                     Legalizer legalizer)
    : cap_per_dbu_(cap_per_dbu),
      max_wire_length_(max_wire_length),
      selector_(std::move(selector)),
      legalizer_(std::move(legalizer))
{
  if (!std::isfinite(cap_per_dbu) || cap_per_dbu < 0
      || std::isnan(max_wire_length) || max_wire_length <= 0 || !selector_) {
    throw std::invalid_argument("Invalid bottom-up buffering parameters");
  }
}

BufferingLoad BottomUpBuffering::load(const std::vector<BufferingNode>& nodes,
                                      size_t driver) const
{
  BufferingLoad result;
  const auto& node = nodes.at(driver);
  result.fanout = node.children.size();
  for (const size_t child : node.children) {
    const auto& sink = nodes.at(child);
    const double length = distance(node.point, sink.point);
    result.pin_cap += sink.input_cap;
    result.wire_cap += length * cap_per_dbu_;
    result.max_wire_length = std::max(result.max_wire_length, length);
  }
  return result;
}

std::vector<BufferingNode> BottomUpBuffering::run(
    std::vector<BufferingNode> nodes,
    size_t root)
{
  expansion_ = {};
  if (nodes.empty() || root >= nodes.size()) {
    throw std::invalid_argument("Bottom-up buffering requires a root");
  }
  std::vector<bool> visited(nodes.size(), false);
  std::vector<size_t> pending{root};
  while (!pending.empty()) {
    const size_t index = pending.back();
    pending.pop_back();
    if (index >= nodes.size() || visited[index]) {
      throw std::invalid_argument("Bottom-up buffering requires a tree");
    }
    visited[index] = true;
    const auto& node = nodes[index];
    if (!std::isfinite(node.point.x) || !std::isfinite(node.point.y)
        || !std::isfinite(node.input_cap) || node.input_cap < 0
        || (node.sink && !node.children.empty())
        || (!node.sink && node.children.empty())) {
      throw std::invalid_argument("Invalid bottom-up buffering node");
    }
    for (const size_t child : node.children) {
      pending.push_back(child);
    }
  }
  if (std::ranges::find(visited, false) != visited.end()) {
    throw std::invalid_argument("Unconnected bottom-up buffering node");
  }
  node_limit_ = nodes.size() * 64;
  nodes_ = std::move(nodes);
  buffer(root, 0);
  return std::move(nodes_);
}

void BottomUpBuffering::buffer(size_t index, unsigned depth)
{
  if (depth >= 64) {
    throw std::runtime_error("Bottom-up buffering exceeded its depth limit");
  }
  if (nodes_[index].sink) {
    return;
  }
  // Insertion can grow nodes_, so never keep a reference across recursion.
  const auto children = nodes_[index].children;
  for (const size_t child : children) {
    buffer(child, depth + 1);
  }
  const auto driven_load = load(nodes_, index);
  if (driven_load.max_wire_length <= max_wire_length_) {
    if (auto choice = selector_(driven_load, nodes_[index].fixed_master)) {
      if (choice->master.empty() || !std::isfinite(choice->input_cap)
          || choice->input_cap < 0) {
        throw std::runtime_error("Invalid bottom-up buffering cell choice");
      }
      nodes_[index].master = choice->master;
      nodes_[index].input_cap = choice->input_cap;
      return;
    }
  }
  const bool length = driven_load.max_wire_length > max_wire_length_;
  if (expansion_.length_splits + expansion_.electrical_splits == 0) {
    expansion_.first_node = index;
    expansion_.first_length = length;
    expansion_.first_load = driven_load;
  }
  if (length) {
    ++expansion_.length_splits;
  } else {
    ++expansion_.electrical_splits;
  }
  split(index, depth);
}

size_t BottomUpBuffering::addStage(BufferingPoint point,
                                   std::vector<size_t> children)
{
  if (nodes_.size() >= node_limit_) {
    throw std::runtime_error("Bottom-up buffering exceeded its stage limit");
  }
  if (legalizer_) {
    point = legalizer_(point);
  }
  if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
    throw std::runtime_error("Invalid bottom-up buffer location");
  }
  BufferingNode node;
  node.point = point;
  node.children = std::move(children);
  nodes_.push_back(std::move(node));
  return nodes_.size() - 1;
}

void BottomUpBuffering::split(size_t index, unsigned depth)
{
  const auto point = nodes_[index].point;
  auto children = nodes_[index].children;
  const auto longest
      = std::ranges::max_element(children, {}, [&](size_t child) {
          return distance(point, nodes_[child].point);
        });
  const size_t farthest = *longest;
  const double length = distance(point, nodes_[farthest].point);
  if (children.size() == 1 || length > max_wire_length_) {
    if (length < 1) {
      throw std::runtime_error("No CTS buffer can drive a sink's capacitance");
    }
    const size_t stage
        = addStage(midpoint(point, nodes_[farthest].point), {farthest});
    *longest = stage;
    nodes_[index].children = std::move(children);
  } else {
    // Split excessive fanout/load spatially. Keep the original H-tree
    // branch point, adding only electrical stages below it.
    std::ranges::sort(children, [&](size_t a, size_t b) {
      const auto pa = nodes_[a].point;
      const auto pb = nodes_[b].point;
      return pa.x == pb.x ? pa.y < pb.y : pa.x < pb.x;
    });
    std::vector<size_t> stages;
    const size_t middle = children.size() / 2;
    for (const auto& [begin, end] :
         {std::pair<size_t, size_t>{0, middle},
          std::pair<size_t, size_t>{middle, children.size()}}) {
      BufferingPoint center;
      for (size_t i = begin; i < end; ++i) {
        center.x += nodes_[children[i]].point.x / (end - begin);
        center.y += nodes_[children[i]].point.y / (end - begin);
      }
      stages.push_back(
          addStage(center, {children.begin() + begin, children.begin() + end}));
    }
    nodes_[index].children = std::move(stages);
  }
  buffer(index, depth + 1);
}

}  // namespace cts

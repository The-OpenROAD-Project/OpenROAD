// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#include "BottleneckAnalysis.hh"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

#include "db_sta/dbSta.hh"
#include "sta/Graph.hh"
#include "sta/MinMax.hh"
#include "sta/Network.hh"
#include "sta/TimingRole.hh"

namespace rsz {

namespace {

// The fanin edges that carry data arrivals: no timing checks, no clock-to-Q
// or latch D-to-Q boundaries, no disabled loops.
template <typename Fn>
void forEachDataFanin(const sta::Graph* graph, sta::Vertex* vertex, Fn&& fn)
{
  sta::VertexInEdgeIterator edge_iter(vertex, graph);
  while (edge_iter.hasNext()) {
    sta::Edge* edge = edge_iter.next();
    const sta::TimingRole* role = edge->role();
    if (role->isTimingCheck() || role == sta::TimingRole::regClkToQ()
        || role->isLatchDtoQ() || edge->isDisabledLoop()) {
      continue;
    }
    fn(edge->from(graph));
  }
}

constexpr size_t kBitsPerWord = 64;

}  // namespace

BottleneckAnalysis::BottleneckAnalysis(sta::dbSta* sta,
                                       const sta::MinMax* min_max)
    : sta_(sta), min_max_(min_max)
{
}

void BottleneckAnalysis::analyze(
    const std::span<const EndpointSlack> violating_endpoints,
    const float slack_margin,
    const int max_endpoints,
    const int max_sources)
{
  for (const Node& node : nodes_) {
    node_of_[node.id] = -1;
  }
  nodes_.clear();
  order_.clear();
  max_cone_need_ = 0.0f;

  const std::vector<size_t> endpoint_nodes = collectCone(
      violating_endpoints.first(std::min(violating_endpoints.size(),
                                         static_cast<size_t>(max_endpoints))),
      slack_margin);
  if (nodes_.empty()) {
    return;
  }

  sumFanoutNeeds(endpoint_nodes);
  sumFaninNeeds(max_sources);
  for (const Node& node : nodes_) {
    max_cone_need_ = std::max(max_cone_need_, coneNeed(node));
  }
}

float BottleneckAnalysis::weightedScore(
    const float local_score,
    sta::Vertex* vertex,
    const BatchPriorityConfig& priority) const
{
  const int32_t node_index = nodeOf(vertex);
  const float weight = node_index >= 0 && max_cone_need_ > 0.0f
                           ? coneNeed(nodes_[node_index]) / max_cone_need_
                           : 0.0f;
  // The factor also multiplies a negative score, so a cone-central target with
  // a loss ranks lower than the same loss elsewhere.
  return local_score
         * (1.0f + priority.alpha * std::pow(weight, priority.gamma));
}

size_t BottleneckAnalysis::admit(sta::Vertex* vertex, const float need)
{
  const size_t node_index = nodes_.size();
  const sta::VertexId id = sta_->graph()->id(vertex);
  // Vertex ids start at 1 and a deleted vertex leaves its id unused, so an
  // id can reach or pass the vertex count.
  if (id >= node_of_.size()) {
    node_of_.resize(id + 1, -1);
  }
  nodes_.push_back({.vertex = vertex, .id = id, .need = need});
  node_of_[id] = static_cast<int32_t>(node_index);
  return node_index;
}

int32_t BottleneckAnalysis::nodeOf(const sta::Vertex* vertex) const
{
  if (vertex == nullptr) {
    return -1;
  }
  const sta::VertexId id = sta_->graph()->id(vertex);
  return id < node_of_.size() ? node_of_[id] : -1;
}

std::vector<size_t> BottleneckAnalysis::collectCone(
    const std::span<const EndpointSlack> endpoints,
    const float slack_margin)
{
  const sta::Graph* graph = sta_->graph();
  std::vector<size_t> endpoint_nodes;
  std::vector<size_t> queue;
  for (const EndpointSlack& endpoint : endpoints) {
    sta::Vertex* vertex = graph->pinLoadVertex(endpoint.first);
    const float need = slack_margin - sta::delayAsFloat(endpoint.second);
    if (vertex == nullptr || need <= 0.0f || nodeOf(vertex) >= 0) {
      continue;
    }
    endpoint_nodes.push_back(admit(vertex, need));
    queue.push_back(endpoint_nodes.back());
  }

  // Backward search through the vertices that still violate: every vertex
  // on a violating path has negative slack itself.
  while (!queue.empty()) {
    const size_t node_index = queue.back();
    queue.pop_back();
    forEachDataFanin(graph, nodes_[node_index].vertex, [&](sta::Vertex* fanin) {
      if (nodeOf(fanin) >= 0) {
        nodes_[node_index].has_cone_fanin = true;
        return;
      }
      const float need
          = slack_margin - sta::delayAsFloat(sta_->slack(fanin, min_max_));
      if (need <= 0.0f) {
        return;
      }
      queue.push_back(admit(fanin, need));
      nodes_[node_index].has_cone_fanin = true;
    });
  }

  // The sums run in level order: a fanin always has the lower level.
  order_.resize(nodes_.size());
  std::iota(order_.begin(), order_.end(), size_t{0});
  std::ranges::stable_sort(order_, [this](const size_t lhs, const size_t rhs) {
    return nodes_[lhs].vertex->level() < nodes_[rhs].vertex->level();
  });
  return endpoint_nodes;
}

void BottleneckAnalysis::sumFanoutNeeds(
    const std::vector<size_t>& endpoint_nodes)
{
  const sta::Graph* graph = sta_->graph();
  const size_t words
      = (endpoint_nodes.size() + kBitsPerWord - 1) / kBitsPerWord;
  std::vector<uint64_t> reach(nodes_.size() * words, 0);
  for (size_t bit = 0; bit < endpoint_nodes.size(); ++bit) {
    reach[endpoint_nodes[bit] * words + bit / kBitsPerWord]
        |= uint64_t{1} << (bit % kBitsPerWord);
  }

  // Descending level: a vertex's set is complete once every fanout of it
  // has been visited, and is then handed to its fanins.
  for (size_t position = order_.size(); position-- > 0;) {
    const size_t node_index = order_[position];
    forEachDataFanin(graph, nodes_[node_index].vertex, [&](sta::Vertex* fanin) {
      const int32_t fanin_index = nodeOf(fanin);
      if (fanin_index < 0) {
        return;
      }
      const size_t fanin_base = static_cast<size_t>(fanin_index) * words;
      const size_t base = node_index * words;
      for (size_t word = 0; word < words; ++word) {
        reach[fanin_base + word] |= reach[base + word];
      }
    });
  }

  for (size_t node_index = 0; node_index < nodes_.size(); ++node_index) {
    Node& node = nodes_[node_index];
    for (size_t word = 0; word < words; ++word) {
      uint64_t bits = reach[node_index * words + word];
      while (bits != 0) {
        const size_t bit = word * kBitsPerWord + std::countr_zero(bits);
        node.fanout_need += nodes_[endpoint_nodes[bit]].need;
        bits &= bits - 1;
      }
    }
  }
}

void BottleneckAnalysis::sumFaninNeeds(const int max_sources)
{
  // The cone sources, worst first.
  std::vector<size_t> sources;
  for (size_t node_index = 0; node_index < nodes_.size(); ++node_index) {
    if (!nodes_[node_index].has_cone_fanin) {
      sources.push_back(node_index);
    }
  }
  std::ranges::stable_sort(sources, [this](const size_t lhs, const size_t rhs) {
    return nodes_[lhs].need > nodes_[rhs].need;
  });
  sources.resize(std::min(sources.size(), static_cast<size_t>(max_sources)));

  const sta::Graph* graph = sta_->graph();
  const size_t words = (sources.size() + kBitsPerWord - 1) / kBitsPerWord;
  std::vector<uint64_t> reach(nodes_.size() * words, 0);
  for (size_t bit = 0; bit < sources.size(); ++bit) {
    reach[sources[bit] * words + bit / kBitsPerWord] |= uint64_t{1}
                                                        << (bit % kBitsPerWord);
  }

  // Ascending level: the fanins are complete before the vertex collects
  // them.
  for (const size_t node_index : order_) {
    forEachDataFanin(graph, nodes_[node_index].vertex, [&](sta::Vertex* fanin) {
      const int32_t fanin_index = nodeOf(fanin);
      if (fanin_index < 0) {
        return;
      }
      const size_t fanin_base = static_cast<size_t>(fanin_index) * words;
      const size_t base = node_index * words;
      for (size_t word = 0; word < words; ++word) {
        reach[base + word] |= reach[fanin_base + word];
      }
    });
  }

  for (size_t node_index = 0; node_index < nodes_.size(); ++node_index) {
    Node& node = nodes_[node_index];
    for (size_t word = 0; word < words; ++word) {
      uint64_t bits = reach[node_index * words + word];
      while (bits != 0) {
        const size_t bit = word * kBitsPerWord + std::countr_zero(bits);
        node.fanin_need += nodes_[sources[bit]].need;
        bits &= bits - 1;
      }
    }
  }
}

float BottleneckAnalysis::coneNeed(const Node& node) const
{
  // The geometric mean keeps the weight in seconds and balances the two
  // sides, so neither the sources nor the endpoints dominate.
  return std::sqrt(node.fanin_need * node.fanout_need);
}

}  // namespace rsz

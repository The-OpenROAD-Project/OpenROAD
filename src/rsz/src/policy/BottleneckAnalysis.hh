// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026-2026, The OpenROAD Authors

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "OptimizationPolicyConfig.hh"
#include "sta/Delay.hh"
#include "sta/GraphClass.hh"

namespace sta {
class dbSta;
class MinMax;
class Pin;
class Vertex;
}  // namespace sta

namespace rsz {

// Ranks shared violating-cone bottlenecks during stalled batches.
// analyze() walks the fanin of the worst violating endpoints through
// negative-slack vertices, stopping at registers and latches.  For each
// vertex, fanout_need sums the need (margin - slack) of the distinct
// endpoints behind it, and fanin_need that of the distinct cone sources in
// front of it, so vertices near the sources are not favored.  Each endpoint
// or source costs one bit per cone vertex, so both counts are capped.
class BottleneckAnalysis
{
 public:
  using EndpointSlack = std::pair<const sta::Pin*, sta::Slack>;

  BottleneckAnalysis(sta::dbSta* sta, const sta::MinMax* min_max);

  // Rebuild the weights from the current timing.
  void analyze(std::span<const EndpointSlack> violating_endpoints,
               float slack_margin,
               int max_endpoints,
               int max_sources);
  // The rank of a target driven from vertex: local_score * (1 + alpha *
  // w^gamma), where w is the vertex's cone need relative to the largest one
  // (0 outside the cone).
  float weightedScore(float local_score,
                      sta::Vertex* vertex,
                      const BatchPriorityConfig& priority) const;
  size_t coneSize() const { return nodes_.size(); }

 private:
  // Need accumulated at one graph vertex from its fanin and fanout cones.
  struct Node
  {
    sta::Vertex* vertex{nullptr};
    sta::VertexId id{0};
    float need{0.0f};
    bool has_cone_fanin{false};
    float fanin_need{0.0f};
    float fanout_need{0.0f};
  };

  // Inserts or updates a vertex in the analyzed cone.
  size_t admit(sta::Vertex* vertex, float need);
  // The node of a live vertex, or -1 outside the cone.
  int32_t nodeOf(const sta::Vertex* vertex) const;
  // The cone behind the endpoints; returns the node of each endpoint.
  std::vector<size_t> collectCone(std::span<const EndpointSlack> endpoints,
                                  float slack_margin);
  // Accumulates distinct downstream endpoint need at every cone vertex.
  void sumFanoutNeeds(const std::vector<size_t>& endpoint_nodes);
  // Accumulates distinct upstream source need to normalize source proximity.
  void sumFaninNeeds(int max_sources);
  // Returns the balanced fanin/fanout need used for ranking.
  float coneNeed(const Node& node) const;

  sta::dbSta* sta_;
  const sta::MinMax* min_max_;
  std::vector<Node> nodes_;
  // nodes_ indices in ascending level order.
  std::vector<size_t> order_;
  // Node index per graph vertex id, -1 outside the cone.  Only the entries of
  // nodes_ are set, and they are cleared before the next analysis.
  std::vector<int32_t> node_of_;
  float max_cone_need_{0.0f};
};

}  // namespace rsz

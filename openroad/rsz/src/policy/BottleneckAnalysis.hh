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

// Ranks shared violating-cone bottlenecks when saved batches stop improving
// WNS.  A vertex's slack deficit is slack_margin - slack.  The analysis sums
// distinct endpoint deficits downstream and startpoint deficits upstream,
// then balances the two so vertices near either end are not favored.  The
// traversal stops at register and latch boundaries; endpoint and startpoint
// counts are capped to bound the per-vertex bitsets.
class BottleneckAnalysis
{
 public:
  using EndpointSlack = std::pair<const sta::Pin*, sta::Slack>;

  BottleneckAnalysis(sta::dbSta* sta, const sta::MinMax* min_max);

  // Rebuild bottleneck severities from the current timing.
  void analyze(std::span<const EndpointSlack> violating_endpoints,
               float slack_margin,
               int max_endpoints,
               int max_startpoints);
  // The rank of a target driven from vertex: local_score * (1 +
  // bottleneck_scale * severity_ratio^bottleneck_exponent).
  // The severity ratio is zero outside the violating cone.
  float bottleneckPriorityScore(float local_score,
                                sta::Vertex* vertex,
                                const BatchPriorityConfig& priority) const;
  size_t coneSize() const { return nodes_.size(); }

 private:
  // The unit tests (TestResizerMt) reach the internals through this peer.
  friend class ResizerTestPeer;

  // Slack deficit and accumulated deficits at one violating-cone vertex.
  struct Node
  {
    sta::Vertex* vertex{nullptr};
    sta::VertexId id{0};
    float slack_deficit{0.0f};
    bool has_cone_fanin{false};
    float startpoint_deficit_sum{0.0f};
    float endpoint_deficit_sum{0.0f};
  };

  // Inserts or updates a vertex in the analyzed cone.
  size_t admit(sta::Vertex* vertex, float slack_deficit);
  // The node of a live vertex, or -1 outside the cone.
  int32_t nodeOf(const sta::Vertex* vertex) const;
  // The cone behind the endpoints; returns the node of each endpoint.
  std::vector<size_t> collectCone(std::span<const EndpointSlack> endpoints,
                                  float slack_margin);
  // Accumulates distinct downstream endpoint deficits at every cone vertex.
  void sumEndpointDeficits(const std::vector<size_t>& endpoint_nodes);
  // Accumulates distinct upstream startpoint deficits at every cone vertex.
  void sumStartpointDeficits(int max_startpoints);
  // Returns the balanced deficit severity used for ranking.
  float bottleneckSeverity(const Node& node) const;

  sta::dbSta* sta_;
  const sta::MinMax* min_max_;
  std::vector<Node> nodes_;
  // nodes_ indices in ascending level order.
  std::vector<size_t> order_;
  // Node index per graph vertex id, -1 outside the cone.  Only the entries of
  // nodes_ are set, and they are cleared before the next analysis.
  std::vector<int32_t> node_of_;
  float max_bottleneck_severity_{0.0f};
};

}  // namespace rsz

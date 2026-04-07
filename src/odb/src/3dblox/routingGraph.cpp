// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "routingGraph.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/unfoldedModel.h"

namespace odb {

// ---------------------------------------------------------------------------
// buildRoutingGraph
//
// Builds a RoutingGraph in Compressed Sparse Row (CSR) format from the
// UnfoldedModel in four passes:
//
//   Pass 1 – assigns a dense uint32_t index to every UnfoldedRegion and
//             builds the pointer-to-index translation map.
//   Pass 2 – collects two kinds of edges and tallies per-node degrees into
//             offsets[u+1]:
//               (a) Inter-chip edges — one per UnfoldedConnection.
//                   Null-endpoint (chip-to-ground virtual) connections are
//                   skipped.
//               (b) Intra-chip complete clique — for blackbox chips (no
//                   internal routing detail) the spec requires all regions
//                   to be treated as mutually reachable.  N*(N-1)/2 edges
//                   are added per blackbox chip; for typical chips with two
//                   regions (front + back) that is exactly one extra edge.
//   Pass 3 – prefix-sums offsets[] to produce CSR row-start pointers.
//   Pass 4 – fills the flat neighbors[] array using a per-node write cursor.
//
// The translation map is returned alongside the graph so callers can convert
// UnfoldedRegion* to node IDs without re-scanning the chip hierarchy.
//
// Complexity: O(V + E) time and space.
// CSR layout: neighbors of node u occupy neighbors[offsets[u]..offsets[u+1]).
// All adjacency data lives in two contiguous vectors — no per-node heap alloc.
// Duplicate edges from multiple connections across one interface are harmless;
// BFS callers skip revisited nodes via their visited[] bitmask.
// ---------------------------------------------------------------------------
std::pair<RoutingGraph, std::unordered_map<const UnfoldedRegion*, uint32_t>>
buildRoutingGraph(const UnfoldedModel& model)
{
  std::unordered_map<const UnfoldedRegion*, uint32_t> region_to_idx;

  // Pass 1: assign dense indices.
  uint32_t idx = 0;
  for (const auto& chip : model.getChips()) {
    for (const auto& region : chip.regions) {
      region_to_idx[&region] = idx++;
    }
  }

  RoutingGraph g;
  g.num_nodes = idx;
  g.offsets.resize(idx + 1, 0);

  if (idx == 0) {
    return {std::move(g), std::move(region_to_idx)};
  }

  // Pass 2: collect edges; tally degrees into offsets[u+1] / offsets[v+1].
  struct Edge
  {
    uint32_t u, v;
  };
  std::vector<Edge> edges;
  edges.reserve(model.getConnections().size());

  // (a) Inter-chip edges from explicit connections.
  for (const auto& conn : model.getConnections()) {
    if (!conn.top_region || !conn.bottom_region) {
      continue;  // Virtual (ground) connection — no region-to-region edge.
    }
    const auto it_top = region_to_idx.find(conn.top_region);
    const auto it_bot = region_to_idx.find(conn.bottom_region);
    if (it_top == region_to_idx.end() || it_bot == region_to_idx.end()) {
      continue;
    }
    const uint32_t u = it_top->second;
    const uint32_t v = it_bot->second;
    edges.push_back({u, v});
    g.offsets[u + 1]++;
    g.offsets[v + 1]++;
  }

  // (b) Intra-chip complete clique for blackbox chips.
  for (const auto& chip : model.getChips()) {
    if (!chip.is_blackbox) {
      continue;
    }
    const auto& regions = chip.regions;
    for (size_t a = 0; a < regions.size(); ++a) {
      const auto it_a = region_to_idx.find(&regions[a]);
      if (it_a == region_to_idx.end()) {
        continue;
      }
      for (size_t b = a + 1; b < regions.size(); ++b) {
        const auto it_b = region_to_idx.find(&regions[b]);
        if (it_b == region_to_idx.end()) {
          continue;
        }
        const uint32_t u = it_a->second;
        const uint32_t v = it_b->second;
        edges.push_back({u, v});
        g.offsets[u + 1]++;
        g.offsets[v + 1]++;
      }
    }
  }

  // Pass 3: prefix-sum degree counts → CSR row-start offsets.
  for (uint32_t i = 1; i <= idx; ++i) {
    g.offsets[i] += g.offsets[i - 1];
  }

  // Pass 4: fill neighbors[] using a per-node write cursor.
  g.neighbors.resize(g.offsets[idx]);
  std::vector<uint32_t> cursor(g.offsets.begin(), g.offsets.begin() + idx);
  for (const auto& [u, v] : edges) {
    g.neighbors[cursor[u]++] = v;
    g.neighbors[cursor[v]++] = u;
  }

  return {std::move(g), std::move(region_to_idx)};
}

}  // namespace odb

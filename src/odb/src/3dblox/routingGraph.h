// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

namespace odb {

class UnfoldedModel;
struct UnfoldedRegion;

// Flat adjacency-list routing graph in Compressed Sparse Row (CSR) format.
// Node IDs are dense uint32_t indices assigned in region-iteration order.
// The adjacency slice for node u is neighbors[offsets[u] .. offsets[u+1]).
struct RoutingGraph
{
  std::vector<uint32_t> offsets;    // size num_nodes+1; CSR row pointers
  std::vector<uint32_t> neighbors;  // all adjacency lists concatenated
  uint32_t num_nodes = 0;
};

// Builds a RoutingGraph (CSR format) from the model in O(V+E) time and space.
// Returns the graph together with the pointer-to-index translation map so
// callers can convert UnfoldedRegion* to dense node IDs without an extra pass.
// Null-endpoint connections (chip-to-ground virtuals) are skipped.
std::pair<RoutingGraph, std::unordered_map<const UnfoldedRegion*, uint32_t>>
buildRoutingGraph(const UnfoldedModel& model);

}  // namespace odb

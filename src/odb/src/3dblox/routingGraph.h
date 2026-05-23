// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <boost/graph/compressed_sparse_row_graph.hpp>
#include <cstddef>
#include <unordered_map>
#include <utility>

#include "utl/Logger.h"

namespace odb {

class UnfoldedModel;
struct UnfoldedRegion;

using RoutingGraphVetex_t = std::size_t;
using RoutingGraph
    = boost::compressed_sparse_row_graph<boost::directedS, RoutingGraphVetex_t>;

// Builds a RoutingGraph (CSR format) from the model in O(V+E) time and space.
// Returns the graph together with the pointer-to-index translation map so
// callers can convert UnfoldedRegion* to dense node IDs without an extra pass.
// Null-endpoint connections (chip-to-ground virtuals) are skipped.
std::pair<RoutingGraph,
          std::unordered_map<const UnfoldedRegion*, RoutingGraphVetex_t>>
buildRoutingGraph(const UnfoldedModel& model, utl::Logger& log);

}  // namespace odb

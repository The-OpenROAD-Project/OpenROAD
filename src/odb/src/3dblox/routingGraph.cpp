// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "routingGraph.h"

#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"

namespace odb {

namespace {
using Edge = std::pair<RoutingGraphVetex_t, RoutingGraphVetex_t>;

void bipartite(const std::vector<const UnfoldedRegion*>& regions_u,
               const std::vector<const UnfoldedRegion*>& regions_v,
               utl::Logger& log,
               const UnfoldedChip& chip,
               const std::unordered_map<const UnfoldedRegion*,
                                        RoutingGraphVetex_t>& region_to_idx,
               std::vector<Edge>& edges)
{
  for (const auto region_u : regions_u) {
    const auto it_a = region_to_idx.find(region_u);
    if (it_a == region_to_idx.end()) {
      log.error(utl::ODB,
                481,
                "Chip '{}' region '{}' is missing from routing graph index — "
                "this is a bug in the unfolded model",
                chip.name,
                region_u->region_inst->getChipRegion()->getName());
      continue;
    }
    for (const auto region_v : regions_v) {
      if (region_u == region_v) {
        continue;
      }
      const auto it_b = region_to_idx.find(region_v);
      if (it_b == region_to_idx.end()) {
        log.error(utl::ODB,
                  534,
                  "Chip '{}' region '{}' is missing from routing graph index — "
                  "this is a bug in the unfolded model",
                  chip.name,
                  region_v->region_inst->getChipRegion()->getName());
        continue;
      }
      const RoutingGraphVetex_t u = it_a->second;
      const RoutingGraphVetex_t v = it_b->second;
      edges.emplace_back(u, v);
      edges.emplace_back(v, u);
    }
  }
}

}  // namespace

// ---------------------------------------------------------------------------
// buildRoutingGraph
//
// Builds a RoutingGraph in Compressed Sparse Row (CSR) format from the
// UnfoldedModel in 3 passes:
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
//   Pass 3 – construct the graph.
//
// The translation map is returned alongside the graph so callers can convert
// UnfoldedRegion* to node IDs without re-scanning the chip hierarchy.
//
// Complexity: O(V + E) time and space.
// ---------------------------------------------------------------------------
std::pair<RoutingGraph,
          std::unordered_map<const UnfoldedRegion*, RoutingGraphVetex_t>>
buildRoutingGraph(const UnfoldedModel& model, utl::Logger& log)
{
  std::unordered_map<const UnfoldedRegion*, RoutingGraphVetex_t> region_to_idx;

  // Pass 1: assign dense indices.
  RoutingGraphVetex_t idx = 0;
  for (const auto& chip : model.getChips()) {
    for (const auto& region : chip.regions) {
      region_to_idx[&region] = idx++;
    }
  }

  RoutingGraph g;

  if (idx == 0) {
    return {std::move(g), std::move(region_to_idx)};
  }

  // Pass 2: collect edges; tally degrees into offsets[u+1] / offsets[v+1].
  std::vector<Edge> edges;
  edges.reserve(model.getConnections().size());

  // (a) Inter-chip edges from explicit connections.
  for (const auto& conn : model.getConnections()) {
    if (!conn.top_region) {
      log.error(utl::ODB,
                546,
                "Connection '{}' has null top region — this is a bug in the "
                "unfolded model",
                conn.connection->getName());
    }
    if (!conn.bottom_region) {
      continue;  // Virtual (ground) connection — no region-to-region edge.
    }
    const auto it_top = region_to_idx.find(conn.top_region);
    const auto it_bot = region_to_idx.find(conn.bottom_region);
    if (it_top == region_to_idx.end() || it_bot == region_to_idx.end()) {
      log.error(
          utl::ODB,
          547,
          "Connection '{}' references region(s) missing from routing graph "
          "index — this is a bug in the unfolded model",
          conn.connection->getName());
    }
    const RoutingGraphVetex_t u = it_top->second;
    const RoutingGraphVetex_t v = it_bot->second;
    edges.emplace_back(u, v);
    edges.emplace_back(v, u);
  }

  for (const auto& chip : model.getChips()) {
    if (!chip.is_blackbox) {
      continue;
    }
    auto chip_type = chip.chip_inst_path.back()->getMasterChip()->getChipType();
    auto master_chip = chip.chip_inst_path.back()->getMasterChip();
    if (chip_type == dbChip::ChipType::DIE) {
      if (master_chip->isTsv()) {
        std::vector<const UnfoldedRegion*> regions(chip.regions.size());
        for (const auto& region : chip.regions) {
          regions.push_back(&region);
        }
        bipartite(regions, regions, log, chip, region_to_idx, edges);
      } else {
        std::vector<const UnfoldedRegion*> front_regions{};
        for (const auto& region : chip.regions) {
          if (region.region_inst->getChipRegion()->getSide()
              == dbChipRegion::Side::FRONT) {
            front_regions.push_back(&region);
          }
        }
        bipartite(
            front_regions, front_regions, log, chip, region_to_idx, edges);
      }
    } else if (chip_type == dbChip::ChipType::RDL
               || chip_type == dbChip::ChipType::SUBSTRATE) {
      std::vector<const UnfoldedRegion*> front_regions{};
      std::vector<const UnfoldedRegion*> back_regions{};
      std::vector<const UnfoldedRegion*> internal_ext_regions{};
      std::vector<const UnfoldedRegion*> internal_regions{};
      for (const auto& region : chip.regions) {
        switch (region.region_inst->getChipRegion()->getSide()) {
          case dbChipRegion::Side::FRONT:
            front_regions.push_back(&region);
            break;
          case dbChipRegion::Side::BACK:
            back_regions.push_back(&region);
            break;
          case dbChipRegion::Side::INTERNAL_EXT:
            internal_ext_regions.push_back(&region);
            break;
          case dbChipRegion::Side::INTERNAL:
            internal_regions.push_back(&region);
            break;
          default:
            log.error(
                utl::ODB,
                548,
                "Chip '{}' region '{}' has invalid side — this is a bug in "
                "the unfolded model",
                chip.name,
                region.region_inst->getChipRegion()->getName());
        }
      }
      if (internal_regions.empty() && internal_ext_regions.empty()) {
        bipartite(front_regions, back_regions, log, chip, region_to_idx, edges);
      } else {
        bipartite(
            front_regions, internal_regions, log, chip, region_to_idx, edges);
        bipartite(
            back_regions, internal_regions, log, chip, region_to_idx, edges);
        std::vector<const UnfoldedRegion*> front_internal_ext{};
        std::vector<const UnfoldedRegion*> back_internal_ext{};
        for (const auto internal_ext_region : internal_ext_regions) {
          if (front_regions.front()->region_inst->getCuboid().zMin()
                  - internal_ext_region->region_inst->getCuboid().zMax()
              < internal_ext_region->region_inst->getCuboid().zMin()
                    - back_regions.front()->region_inst->getCuboid().zMax()) {
            front_internal_ext.push_back(internal_ext_region);
          } else {
            back_internal_ext.push_back(internal_ext_region);
          }
        }
        bipartite(
            front_regions, front_internal_ext, log, chip, region_to_idx, edges);
        bipartite(
            back_regions, back_internal_ext, log, chip, region_to_idx, edges);
      }
    }
  }

  g = RoutingGraph(
      boost::edges_are_unsorted_multi_pass, edges.begin(), edges.end(), idx);

  return {std::move(g), std::move(region_to_idx)};
}

}  // namespace odb

// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <cmath>
#include <cstddef>
#include <map>
#include <numeric>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbWireGraph.h"
#include "odb/geom.h"
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace odb {

namespace {

const char* sideToString(UnfoldedRegionSide side)
{
  switch (side) {
    case UnfoldedRegionSide::TOP:
      return "TOP";
    case UnfoldedRegionSide::BOTTOM:
      return "BOTTOM";
    case UnfoldedRegionSide::INTERNAL:
      return "INTERNAL";
    case UnfoldedRegionSide::INTERNAL_EXT:
      return "INTERNAL_EXT";
  }
  return "UNKNOWN";
}

MatingSurfaces getMatingSurfaces(const UnfoldedConnection& conn)
{
  auto* r1 = conn.top_region;
  auto* r2 = conn.bottom_region;
  if (!r1 || !r2) {
    return {.valid = false, .top_z = 0, .bot_z = 0};
  }

  // r1 faces down (Bottom side) and r2 faces up (Top side) -> r1 is above r2
  bool r1_down_r2_up = r1->isBottom() && r2->isTop();
  // r1 faces up (Top side) and r2 faces down (Bottom side) -> r2 is above r1
  bool r1_up_r2_down = r1->isTop() && r2->isBottom();

  if (r1_down_r2_up == r1_up_r2_down) {
    return {.valid = false, .top_z = 0, .bot_z = 0};
  }

  auto* top = r1_down_r2_up ? r1 : r2;
  auto* bot = r1_down_r2_up ? r2 : r1;
  return {
      .valid = true, .top_z = top->getSurfaceZ(), .bot_z = bot->getSurfaceZ()};
}

bool isValid(const UnfoldedConnection& conn)
{
  if (!conn.top_region || !conn.bottom_region) {
    return true;
  }
  if (!conn.top_region->cuboid.xyIntersects(conn.bottom_region->cuboid)) {
    return false;
  }
  if (conn.top_region->isInternalExt() || conn.bottom_region->isInternalExt()) {
    return std::max(conn.top_region->cuboid.zMin(),
                    conn.bottom_region->cuboid.zMin())
           <= std::min(conn.top_region->cuboid.zMax(),
                       conn.bottom_region->cuboid.zMax());
  }

  auto surfaces = getMatingSurfaces(conn);
  if (!surfaces.valid) {
    return false;
  }
  if (surfaces.top_z < surfaces.bot_z) {
    return false;
  }
  return (surfaces.top_z - surfaces.bot_z) == conn.connection->getThickness();
}

}  // namespace

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(dbChip* chip, int bump_pitch_tolerance)
{
  UnfoldedModel model(logger_, chip);
  auto* top_cat = dbMarkerCategory::createOrReplace(chip, "3DBlox");

  checkFloatingChips(top_cat, model);
  checkOverlappingChips(top_cat, model);
  checkInternalExtUsage(top_cat, model);
  checkConnectionRegions(top_cat, model);
  checkNetConnectivity(top_cat, model, bump_pitch_tolerance);
}

void Checker::checkFloatingChips(dbMarkerCategory* top_cat,
                                 const UnfoldedModel& model)
{
  const auto& chips = model.getChips();
  // Add one more node for "ground" (external world: package, PCB, ...)
  utl::UnionFind uf(chips.size() + 1);
  const size_t ground_node = chips.size();

  std::unordered_map<const UnfoldedChip*, size_t> chip_map;
  for (size_t i = 0; i < chips.size(); ++i) {
    chip_map[&chips[i]] = i;
  }

  for (const auto& conn : model.getConnections()) {
    if (isValid(conn)) {
      // Case 1: Both regions exist - connect the two chips together
      if (conn.top_region && conn.bottom_region) {
        auto it1 = chip_map.find(conn.top_region->parent_chip);
        auto it2 = chip_map.find(conn.bottom_region->parent_chip);
        if (it1 != chip_map.end() && it2 != chip_map.end()) {
          uf.unite(it1->second, it2->second);
        }
      }
      // Case 2: Virtual connection (one region is null) - connect chip to
      // ground
      else if (conn.top_region || conn.bottom_region) {
        const UnfoldedRegion* region
            = conn.top_region ? conn.top_region : conn.bottom_region;
        auto it = chip_map.find(region->parent_chip);
        if (it != chip_map.end()) {
          uf.unite(it->second, ground_node);
        }
      }
    }
  }

  std::vector<std::vector<const UnfoldedChip*>> groups(chips.size() + 1);
  for (size_t i = 0; i < chips.size(); ++i) {
    groups[uf.find(i)].push_back(&chips[i]);
  }
  auto ground_leader = uf.find(ground_node);
  const bool ground_empty = groups[ground_leader].empty();
  groups.erase(groups.begin() + ground_leader);

  std::erase_if(groups, [](const auto& g) { return g.empty(); });

  std::ranges::sort(
      groups, [](const auto& a, const auto& b) { return a.size() < b.size(); });

  if (ground_empty) {
    logger_->warn(
        utl::ODB,
        206,
        "No ground group found. Erasing biggest group from floating chips.");
    if (!groups.empty()) {
      groups.pop_back();
    }
  }

  if (!groups.empty()) {
    auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Floating chips");
    logger_->warn(utl::ODB, 151, "Found {} floating chip sets", groups.size());
    for (const auto& group : groups | std::views::reverse) {
      auto* marker = dbMarker::create(cat);
      for (auto* chip : group) {
        marker->addShape(chip->cuboid);
        marker->addSource(chip->chip_inst_path.back());
      }
      marker->setComment("Isolated chip set starting with " + group[0]->name);
    }
  }
}

void Checker::checkOverlappingChips(dbMarkerCategory* top_cat,
                                    const UnfoldedModel& model)
{
  const auto& chips = model.getChips();
  std::vector<int> sorted(chips.size());
  std::iota(sorted.begin(), sorted.end(), 0);
  std::ranges::sort(sorted, [&](int a, int b) {
    return chips[a].cuboid.xMin() < chips[b].cuboid.xMin();
  });

  std::vector<std::pair<const UnfoldedChip*, const UnfoldedChip*>> overlaps;
  for (size_t i = 0; i < sorted.size(); ++i) {
    auto* c1 = &chips[sorted[i]];
    for (size_t j = i + 1; j < sorted.size(); ++j) {
      auto* c2 = &chips[sorted[j]];
      if (c2->cuboid.xMin() >= c1->cuboid.xMax()) {
        break;
      }
      if (c1->cuboid.overlaps(c2->cuboid)) {
        overlaps.emplace_back(c1, c2);
      }
    }
  }

  if (!overlaps.empty()) {
    auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Overlapping chips");
    logger_->warn(utl::ODB, 156, "Found {} overlapping chips", overlaps.size());
    for (const auto& [inst1, inst2] : overlaps) {
      auto* marker = dbMarker::create(cat);
      auto intersection = inst1->cuboid.intersect(inst2->cuboid);
      marker->addShape(intersection);
      marker->addSource(inst1->chip_inst_path.back());
      marker->addSource(inst2->chip_inst_path.back());
      marker->setComment(
          fmt::format("Chips {} and {} overlap", inst1->name, inst2->name));
    }
  }
}

void Checker::checkInternalExtUsage(dbMarkerCategory* top_cat,
                                    const UnfoldedModel& model)
{
  dbMarkerCategory* cat = nullptr;
  for (const auto& chip : model.getChips()) {
    for (const auto& region : chip.regions) {
      if (region.isInternalExt() && !region.isUsed) {
        if (!cat) {
          cat = dbMarkerCategory::createOrReplace(top_cat,
                                                  "Unused internal_ext");
        }
        logger_->warn(utl::ODB,
                      464,
                      "Region {} is internal_ext but unused",
                      region.region_inst->getChipRegion()->getName());
        auto* marker = dbMarker::create(cat);
        marker->addSource(region.region_inst);
        marker->addShape(region.cuboid);
        marker->setComment(
            fmt::format("Unused internal_ext region: {}",
                        region.region_inst->getChipRegion()->getName()));
      }
    }
  }
}

void Checker::checkConnectionRegions(dbMarkerCategory* top_cat,
                                     const UnfoldedModel& model)
{
  auto describe = [](const UnfoldedRegion* r, dbMarker* marker) {
    marker->addSource(r->region_inst);
    marker->addShape(Rect(r->cuboid.xMin(),
                          r->cuboid.yMin(),
                          r->cuboid.xMax(),
                          r->cuboid.yMax()));
    return fmt::format("{}/{} (faces {})",
                       r->parent_chip->name,
                       r->region_inst->getChipRegion()->getName(),
                       sideToString(r->effective_side));
  };
  int count = 0;
  dbMarkerCategory* cat = nullptr;
  for (const auto& conn : model.getConnections()) {
    if (!isValid(conn)) {
      if (!cat) {
        cat = dbMarkerCategory::createOrReplace(top_cat, "Connection regions");
      }
      auto* marker = dbMarker::create(cat);
      marker->addSource(conn.connection);
      std::string msg = fmt::format("Invalid connection {}: {} to {}",
                                    conn.connection->getName(),
                                    describe(conn.top_region, marker),
                                    describe(conn.bottom_region, marker));
      marker->setComment(msg);
      logger_->warn(utl::ODB, 207, msg);
      count++;
    }
  }
  if (count > 0) {
    logger_->warn(utl::ODB, 273, "Found {} invalid connections", count);
  }
}

void Checker::checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                         const UnfoldedModel& model)
{
}

void Checker::checkNetConnectivity(dbMarkerCategory* top_cat,
                                   const UnfoldedModel& model,
                                   int bump_pitch_tolerance)
{
  int disconnected_nets = 0;

  // 1. Pre-index connections by region for faster lookup
  // Map: Region -> List of Connections attached to it
  std::unordered_map<const UnfoldedRegion*,
                     std::vector<const UnfoldedConnection*>>
      region_connections;

  for (const auto& conn : model.getConnections()) {
    if (isValid(conn) && conn.top_region && conn.bottom_region) {
      region_connections[conn.top_region].push_back(&conn);
      region_connections[conn.bottom_region].push_back(&conn);
    }
  }

  for (const auto& net : model.getNets()) {
    if (net.connected_bumps.size() < 2) {
      continue;
    }

    utl::UnionFind uf(net.connected_bumps.size());
    std::unordered_map<const UnfoldedRegion*, std::vector<int>> region_bumps;

    checkIntraChipConnectivity(net, uf, region_bumps);

    checkInterChipConnectivity(
        net, uf, region_bumps, region_connections, bump_pitch_tolerance);

    // 4. Analyze groups and report violations
    std::vector<std::vector<int>> groups(net.connected_bumps.size());
    for (size_t i = 0; i < net.connected_bumps.size(); ++i) {
      groups[uf.find((int) i)].push_back((int) i);
    }
    std::erase_if(groups, [](const auto& g) { return g.empty(); });

    if (groups.size() > 1) {
      auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Open nets");
      disconnected_nets++;
      // Heuristic: Assume largest group is "main" net
      auto max_it = std::ranges::max_element(
          groups, [](auto& a, auto& b) { return a.size() < b.size(); });
      if (max_it != groups.end()) {
        groups.erase(max_it);
      }

      auto* marker = dbMarker::create(cat);
      marker->addSource(net.chip_net);

      int disconnected_count = 0;
      for (auto& group : groups) {
        for (int idx : group) {
          disconnected_count++;
          auto* b = net.connected_bumps[idx];
          if (b->bump_inst) {
            marker->addSource(b->bump_inst);
            marker->addShape(
                Rect(b->global_position.x() - kBumpMarkerHalfSize,
                     b->global_position.y() - kBumpMarkerHalfSize,
                     b->global_position.x() + kBumpMarkerHalfSize,
                     b->global_position.y() + kBumpMarkerHalfSize));
          }
        }
      }
      marker->setComment(
          fmt::format("Net {} has {} disconnected bump(s) out of {} total.",
                      net.chip_net->getName(),
                      disconnected_count,
                      net.connected_bumps.size()));
    }
  }

  if (disconnected_nets > 0) {
    logger_->warn(
        utl::ODB, 405, "Found {} disconnected nets.", disconnected_nets);
  }
}

void Checker::checkIntraChipConnectivity(
    const UnfoldedNet& net,
    utl::UnionFind& uf,
    std::unordered_map<const UnfoldedRegion*, std::vector<int>>& region_bumps)
{
  std::map<UnfoldedChip*, std::vector<int>> chip_groups;
  for (size_t i = 0; i < net.connected_bumps.size(); ++i) {
    auto* b = net.connected_bumps[i];
    chip_groups[b->parent_region->parent_chip].push_back((int) i);
    region_bumps[b->parent_region].push_back((int) i);
  }

  for (const auto& [chip, indices] : chip_groups) {
    verifyChipConnectivity(indices, net, uf);
  }
}

void Checker::checkInterChipConnectivity(
    const UnfoldedNet& net,
    utl::UnionFind& uf,
    const std::unordered_map<const UnfoldedRegion*, std::vector<int>>&
        region_bumps,
    const std::unordered_map<const UnfoldedRegion*,
                             std::vector<const UnfoldedConnection*>>&
        region_connections,
    int bump_pitch_tolerance)
{
  // Iterate only over regions that actually have bumps for this net
  for (const auto& [region, bumps] : region_bumps) {
    auto conn_it = region_connections.find(region);
    if (conn_it == region_connections.end()) {
      continue;
    }

    for (const auto* conn : conn_it->second) {
      const UnfoldedRegion* other_region = (conn->top_region == region)
                                               ? conn->bottom_region
                                               : conn->top_region;

      auto other_bumps_it = region_bumps.find(other_region);
      // Enforce a canonical processing direction: only merge from the
      // top-region perspective. Since every connection is encountered twice in
      // the region loop (once as top, once as bottom), we skip the bottom-side
      // visit to avoid redundant work.
      if (other_bumps_it != region_bumps.end() && conn->top_region == region) {
        connectBumpsBetweenRegions(
            bumps, other_bumps_it->second, bump_pitch_tolerance, net, uf);
      }
    }
  }
}

void Checker::verifyChipConnectivity(const std::vector<int>& indices,
                                     const UnfoldedNet& net,
                                     utl::UnionFind& uf)
{
  if (indices.size() < 2) {
    return;
  }

  // Identify the dbNet and valid BTerms for these bumps
  dbNet* local_net = nullptr;
  std::vector<std::pair<int, dbBTerm*>> valid_bumps;
  valid_bumps.reserve(indices.size());

  bool has_valid_terms = false;

  for (int idx : indices) {
    auto* b = net.connected_bumps[idx];
    dbBTerm* term = nullptr;
    if (b->bump_inst) {
      if (auto* cb = b->bump_inst->getChipBump()) {
        term = cb->getBTerm();
      }
    }

    if (term) {
      valid_bumps.emplace_back(idx, term);
      if (!local_net) {
        local_net = term->getNet();
      }
      has_valid_terms = true;
    }
  }

  if (!has_valid_terms || !local_net || !local_net->getWire()) {
    return;  // No wire or terms -> assume open
  }

  // Build graph to analyze connectivity
  dbWireGraph graph;
  graph.decode(local_net->getWire());

  auto find_root = [](dbWireGraph::Node* n) {
    while (n->in_edge()) {
      n = n->in_edge()->source();
    }
    return n;
  };

  // Map: Component Root -> List of Connected Bump Indices
  std::map<dbWireGraph::Node*, std::vector<int>> components;

  // Iterate graph nodes to find which component each BTerm belongs to.
  // We must visit ALL nodes because a wire may wander far outside the
  // bounding box of its terminals before coming back (e.g. detour routing).
  for (auto it = graph.begin_nodes(); it != graph.end_nodes(); ++it) {
    dbWireGraph::Node* node = *it;
    int x, y;
    node->xy(x, y);
    Point pt(x, y);

    dbTechLayer* layer = node->layer();

    for (const auto& [idx, term] : valid_bumps) {
      bool match = false;
      for (dbBPin* pin : term->getBPins()) {
        for (dbBox* box : pin->getBoxes()) {
          if (box->getTechLayer() == layer && box->getBox().intersects(pt)) {
            match = true;
            break;
          }
        }
        if (match) {
          break;
        }
      }

      if (match) {
        dbWireGraph::Node* root = find_root(node);
        components[root].push_back(idx);
      }
    }
  }

  // Unite bumps that share the same component root
  for (const auto& [root, bump_list] : components) {
    for (size_t k = 1; k < bump_list.size(); ++k) {
      uf.unite(bump_list[0], bump_list[k]);
    }
  }
}

void Checker::connectBumpsBetweenRegions(const std::vector<int>& set_a,
                                         const std::vector<int>& set_b,
                                         int tolerance,
                                         const UnfoldedNet& net,
                                         utl::UnionFind& uf)
{
  if (tolerance == 0) {
    // Exact match: Use a hash multimap for O(1) lookup
    std::unordered_multimap<std::pair<int, int>,
                            int,
                            boost::hash<std::pair<int, int>>>
        xy_map;
    for (int idx : set_b) {
      const Point3D& p = net.connected_bumps[idx]->global_position;
      xy_map.insert({{p.x(), p.y()}, idx});
    }

    for (int idx : set_a) {
      const Point3D& p = net.connected_bumps[idx]->global_position;
      // Use equal_range to pick up ALL coincident bumps in set_b,
      auto [begin, end] = xy_map.equal_range({p.x(), p.y()});
      for (auto it = begin; it != end; ++it) {
        uf.unite(idx, it->second);
      }
    }
  } else {
    // Tolerance match: Use a spatial sort (sweep-line like)
    // Sort both lists by X coordinate
    auto sort_by_x = [&](int a, int b) {
      return net.connected_bumps[a]->global_position.x()
             < net.connected_bumps[b]->global_position.x();
    };
    // Create local copies to sort
    std::vector<int> sorted_a = set_a;
    std::vector<int> sorted_b = set_b;
    std::ranges::sort(sorted_a, sort_by_x);
    std::ranges::sort(sorted_b, sort_by_x);

    size_t j_start = 0;
    for (int i_idx : sorted_a) {
      const Point3D& p1 = net.connected_bumps[i_idx]->global_position;

      // Advance j_start to the first potential candidate in X range
      while (j_start < sorted_b.size()) {
        const Point3D& p2
            = net.connected_bumps[sorted_b[j_start]]->global_position;
        if (p2.x() >= p1.x() - tolerance) {
          break;
        }
        j_start++;
      }

      // Check candidates in X range
      for (size_t j = j_start; j < sorted_b.size(); ++j) {
        int j_idx = sorted_b[j];
        const Point3D& p2 = net.connected_bumps[j_idx]->global_position;

        if (p2.x() > p1.x() + tolerance) {
          break;  // Out of X range, stop
        }

        if (std::abs(p1.y() - p2.y()) <= tolerance) {
          uf.unite(i_idx, j_idx);
        }
      }
    }
  }
}

}  // namespace odb

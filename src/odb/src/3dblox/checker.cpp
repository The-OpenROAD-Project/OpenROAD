// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/geom.h"
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace odb {

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(dbChip* chip)
{
  UnfoldedModel model(logger_, chip);
  auto* top_cat = dbMarkerCategory::createOrReplace(chip, "3DBlox");

  checkFloatingChips(top_cat, model);
  checkOverlappingChips(top_cat, model);
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
        marker->addShape(Rect(chip->cuboid.xMin(),
                              chip->cuboid.yMin(),
                              chip->cuboid.xMax(),
                              chip->cuboid.yMax()));
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
  std::vector<std::pair<const UnfoldedChip*, const UnfoldedChip*>> overlaps;

  for (size_t i = 0; i < chips.size(); i++) {
    auto cuboid_i = chips[i].cuboid;
    for (size_t j = i + 1; j < chips.size(); j++) {
      auto cuboid_j = chips[j].cuboid;
      if (cuboid_i.overlaps(cuboid_j)) {
        overlaps.emplace_back(&chips[i], &chips[j]);
      }
    }
  }

  if (!overlaps.empty()) {
    auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Overlapping chips");
    logger_->warn(
        utl::ODB, 156, "Found {} overlapping chips", (int) overlaps.size());

    for (const auto& [inst1, inst2] : overlaps) {
      odb::dbMarker* marker = odb::dbMarker::create(cat);

      auto cuboid1 = inst1->cuboid;
      auto cuboid2 = inst2->cuboid;
      auto intersection = cuboid1.intersect(cuboid2);

      odb::Rect bbox(intersection.xMin(),
                     intersection.yMin(),
                     intersection.xMax(),
                     intersection.yMax());
      marker->addShape(bbox);

      marker->addSource(inst1->chip_inst_path.back());
      marker->addSource(inst2->chip_inst_path.back());

      std::string comment
          = "Chips " + inst1->name + " and " + inst2->name + " overlap";
      marker->setComment(comment);
    }
  }
}

void Checker::checkConnectionRegions(dbMarkerCategory* top_cat,
                                     const UnfoldedModel& model)
{
}

void Checker::checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                         const UnfoldedModel& model)
{
}

void Checker::checkNetConnectivity(dbMarkerCategory* top_cat,
                                   const UnfoldedModel& model)
{
}

bool Checker::isOverlapFullyInConnections(const UnfoldedChip* chip1,
                                          const UnfoldedChip* chip2,
                                          const Cuboid& overlap) const
{
  return false;
}

Checker::MatingSurfaces Checker::getMatingSurfaces(
    const UnfoldedConnection& conn) const
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

bool Checker::isValid(const UnfoldedConnection& conn) const
{
  if (!conn.top_region || !conn.bottom_region) {
    return true;
  }
  if (!conn.top_region->cuboid.xyIntersects(conn.bottom_region->cuboid)) {
    return false;
  }
  if (conn.top_region->isInternalExt() || conn.bottom_region->isInternalExt()) {
    return conn.top_region->parent_chip == conn.bottom_region->parent_chip
           && std::max(conn.top_region->cuboid.zMin(),
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

}  // namespace odb

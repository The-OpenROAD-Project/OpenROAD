// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"  // IWYU pragma: keep
#include "odb/unfoldedModel.h"
#include "utl/Logger.h"
#include "utl/spatialIndex.h"
#include "utl/unionFind.h"

namespace odb {

namespace {

constexpr int kBumpMarkerHalfSize = 50;

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

using AlignmentMarkerIndex
    = utl::SpatialIndex<Point, const UnfoldedAlignmentMarker*>;

std::vector<AlignmentMarkerIndex::Value> collectMarkersInRect(
    const std::deque<UnfoldedAlignmentMarker>& markers,
    const Rect& rect)
{
  std::vector<AlignmentMarkerIndex::Value> out;
  for (const auto& m : markers) {
    const Point center = m.getBBox().center();
    if (rect.intersects(center)) {
      out.emplace_back(center, &m);
    }
  }
  return out;
}

using AlignmentViolationReporter
    = std::function<void(const UnfoldedAlignmentMarker*,
                         const UnfoldedAlignmentMarker*,
                         const std::string&)>;

void matchMarkersBetweenChips(
    const UnfoldedModel* model,
    const std::vector<AlignmentMarkerIndex::Value>& list_a,
    const std::vector<AlignmentMarkerIndex::Value>& list_b,
    const UnfoldedChip* c_a,
    const UnfoldedChip* c_b,
    std::unordered_set<const UnfoldedAlignmentMarker*>& matched_markers,
    const AlignmentViolationReporter& report)
{
  AlignmentMarkerIndex index_b(list_b);

  for (auto& [pa, marker] : list_a) {
    int max_tolerance = 0;
    const std::vector<dbAlignmentMarkerRule*>& rules
        = model->getAlignmentMarkerRules(marker->inst->getMaster());
    for (const auto* rule : rules) {
      max_tolerance = std::max(max_tolerance, rule->getTolerance());
    }
    const Rect qbox(pa.x() - max_tolerance,
                    pa.y() - max_tolerance,
                    pa.x() + max_tolerance,
                    pa.y() + max_tolerance);
    auto candidates = index_b.query(qbox);

    std::set<AlignmentMarkerIndex::Value> counterparts;
    for (const auto* rule : rules) {
      for (const auto& candidate : candidates) {
        auto* a_marker = marker;
        auto* b_marker = candidate.second;
        if (rule->getMasterA() != a_marker->inst->getMaster()) {
          std::swap(a_marker, b_marker);
        }
        if (b_marker->inst->getMaster() != rule->getMasterB()) {
          continue;
        }
        if (odb::Point::manhattanDistance(a_marker->getBBox().center(),
                                          b_marker->getBBox().center())
            <= rule->getTolerance()) {
          counterparts.insert(candidate);
          // check the relative orientation
          if (!rule->getRelativeOrientations().empty()) {
            dbTransform rel_xform(a_marker->getOrient());
            rel_xform.invert();
            rel_xform.concat(dbTransform(b_marker->getOrient()));
            const dbOrientType relative_orient = rel_xform.getOrient();
            const auto& allowed_relative_orients
                = rule->getRelativeOrientations();
            if (std::ranges::find(allowed_relative_orients, relative_orient)
                == allowed_relative_orients.end()) {
              // candidate violates the relative-orientation constraint
              report(a_marker,
                     b_marker,
                     fmt::format(
                         "Alignment marker on {} has mismatched "
                         "relative orientation with counterpart on {} ({})",
                         b_marker->parent_chip->name,
                         a_marker->parent_chip->name,
                         relative_orient.getString()));
            }
          }
        }
      }
    }
    if (!counterparts.empty()) {
      matched_markers.insert(marker);
      if (counterparts.size() > 1) {
        report(marker,
               nullptr,
               fmt::format(
                   "Alignment marker on {} has {} counterparts on {} within "
                   "tolerance",
                   marker->parent_chip->name,
                   counterparts.size(),
                   c_b->name));
      }
    }
    for (const auto& counterpart : counterparts) {
      matched_markers.insert(counterpart.second);
      index_b.remove(counterpart);
    }
  }
}
}  // namespace

Checker::Checker(utl::Logger* logger, dbDatabase* db) : logger_(logger), db_(db)
{
}

void Checker::check()
{
  dbChip* chip = db_->getChip();
  const UnfoldedModel* model = db_->getUnfoldedModel();
  if (model == nullptr || chip == nullptr) {
    return;
  }
  auto* top_cat = dbMarkerCategory::createOrReplace(chip, "3DBlox");
  checkLogicalConnectivity(top_cat, model);
  checkFloatingChips(top_cat, model);
  checkOverlappingChips(top_cat, model);
  checkInternalExtUsage(top_cat, model);
  checkConnectionRegions(top_cat, model);
  checkBumpPhysicalAlignment(top_cat, model);
  checkAlignmentMarkers(top_cat, model);
}

void Checker::checkFloatingChips(dbMarkerCategory* top_cat,
                                 const UnfoldedModel* model)
{
  const auto& chips = model->getChips();
  // Add one more node for "ground" (external world: package, PCB, ...)
  utl::UnionFind uf(chips.size() + 1);
  const size_t ground_node = chips.size();

  std::unordered_map<const UnfoldedChip*, size_t> chip_map;
  for (size_t i = 0; i < chips.size(); ++i) {
    chip_map[&chips[i]] = i;
  }

  for (const auto& conn : model->getConnections()) {
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
      // dbMarker::create returns nullptr once the category hits its
      // max_markers_ limit; skip silently in that case to avoid a
      // null-deref crash.
      if (auto* marker = dbMarker::create(cat)) {
        for (auto* chip : group) {
          marker->addShape(chip->cuboid);
          marker->addSource(chip->chip_inst_path.back());
        }
        marker->setComment("Isolated chip set starting with " + group[0]->name);
      }
    }
  }
}

void Checker::checkOverlappingChips(dbMarkerCategory* top_cat,
                                    const UnfoldedModel* model)
{
  const auto& chips = model->getChips();
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
      if (auto* marker = dbMarker::create(cat)) {
        auto intersection = inst1->cuboid.intersect(inst2->cuboid);
        marker->addShape(intersection);
        marker->addSource(inst1->chip_inst_path.back());
        marker->addSource(inst2->chip_inst_path.back());
        marker->setComment(
            fmt::format("Chips {} and {} overlap", inst1->name, inst2->name));
      }
    }
  }
}

void Checker::checkInternalExtUsage(dbMarkerCategory* top_cat,
                                    const UnfoldedModel* model)
{
  dbMarkerCategory* cat = nullptr;
  for (const auto& chip : model->getChips()) {
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
        if (auto* marker = dbMarker::create(cat)) {
          marker->addSource(region.region_inst);
          marker->addShape(region.cuboid);
          marker->setComment(
              fmt::format("Unused internal_ext region: {}",
                          region.region_inst->getChipRegion()->getName()));
        }
      }
    }
  }
}

void Checker::checkConnectionRegions(dbMarkerCategory* top_cat,
                                     const UnfoldedModel* model)
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
  for (const auto& conn : model->getConnections()) {
    if (!isValid(conn)) {
      if (!cat) {
        cat = dbMarkerCategory::createOrReplace(top_cat, "Connection regions");
      }
      if (auto* marker = dbMarker::create(cat)) {
        marker->addSource(conn.connection);
        std::string msg = fmt::format("Invalid connection {}: {} to {}",
                                      conn.connection->getName(),
                                      describe(conn.top_region, marker),
                                      describe(conn.bottom_region, marker));
        marker->setComment(msg);
        logger_->warn(utl::ODB, 207, msg);
      }
      count++;
    }
  }
  if (count > 0) {
    logger_->warn(utl::ODB, 273, "Found {} invalid connections", count);
  }
}

void Checker::checkBumpPhysicalAlignment(dbMarkerCategory* top_cat,
                                         const UnfoldedModel* model)
{
  dbMarkerCategory* cat = nullptr;
  int violation_count = 0;
  for (const auto& chip : model->getChips()) {
    for (const auto& region : chip.regions) {
      for (const auto& bump : region.bumps) {
        const auto& p = bump.global_position;
        if (!region.cuboid.getEnclosingRect().intersects({p.x(), p.y()})) {
          violation_count++;
          if (!cat) {
            cat = dbMarkerCategory::createOrReplace(top_cat, "Bump Alignment");
          }
          // dbMarker::create returns nullptr once the category hits its
          // max_markers_ limit; skip the addSource/addShape/setComment
          // chain to avoid a null-deref crash in that case.
          if (auto* marker = dbMarker::create(cat)) {
            marker->addSource(bump.bump_inst);
            marker->addShape(Rect(p.x() - kBumpMarkerHalfSize,
                                  p.y() - kBumpMarkerHalfSize,
                                  p.x() + kBumpMarkerHalfSize,
                                  p.y() + kBumpMarkerHalfSize));
            marker->setComment(
                fmt::format("Bump is outside its parent region {}",
                            region.region_inst->getChipRegion()->getName()));
          }
        }
      }
    }
  }
  if (violation_count > 0) {
    logger_->warn(utl::ODB,
                  463,
                  "Found {} bump(s) outside their parent region(s)",
                  violation_count);
  }
}

void Checker::checkNetConnectivity(dbMarkerCategory* top_cat,
                                   const UnfoldedModel* model)
{
}

void Checker::checkAlignmentMarkers(dbMarkerCategory* top_cat,
                                    const UnfoldedModel* model)
{
  std::unordered_set<const UnfoldedAlignmentMarker*> matched_markers;
  dbMarkerCategory* cat = nullptr;
  int violation_count = 0;
  auto report = [&](const UnfoldedAlignmentMarker* m,
                    const UnfoldedAlignmentMarker* other,
                    const std::string& msg) {
    if (!cat) {
      cat = dbMarkerCategory::createOrReplace(top_cat, "Alignment Markers");
    }
    auto* marker = dbMarker::create(cat);
    // TODO: Add sources correctly
    marker->addShape(m->getBBox());
    if (other) {
      marker->addShape(other->getBBox());
    }
    marker->setComment(msg);
    violation_count++;
  };

  for (const auto& conn : model->getConnections()) {
    if (!isValid(conn)) {
      continue;
    }
    const UnfoldedRegion* ra = conn.top_region;
    const UnfoldedRegion* rb = conn.bottom_region;
    if (!ra || !rb) {
      continue;
    }
    const UnfoldedChip* c_a = ra->parent_chip;
    const UnfoldedChip* c_b = rb->parent_chip;
    if (c_a == c_b) {
      continue;
    }
    if (c_a->alignment_markers.empty() && c_b->alignment_markers.empty()) {
      continue;
    }

    const Rect overlap = ra->cuboid.getEnclosingRect().intersect(
        rb->cuboid.getEnclosingRect());
    auto list_a = collectMarkersInRect(c_a->alignment_markers, overlap);
    auto list_b = collectMarkersInRect(c_b->alignment_markers, overlap);
    if (list_a.empty() && list_b.empty()) {
      continue;
    }

    matchMarkersBetweenChips(
        model, list_a, list_b, c_a, c_b, matched_markers, report);
  }
  for (const auto& chip : model->getChips()) {
    for (const auto& marker : chip.alignment_markers) {
      if (matched_markers.find(&marker) == matched_markers.end()) {
        report(&marker,
               nullptr,
               fmt::format("Alignment marker on {} has no counterpart",
                           chip.name));
      }
    }
  }

  if (violation_count > 0) {
    logger_->warn(utl::ODB,
                  404,
                  "Found {} alignment marker violation(s)",
                  violation_count);
  }
}

void Checker::checkLogicalConnectivity(dbMarkerCategory* top_cat,
                                       const UnfoldedModel* model)
{
  std::unordered_map<const UnfoldedBump*, const UnfoldedNet*> bump_net_map;
  for (const auto& net : model->getNets()) {
    for (const auto* bump : net.connected_bumps) {
      bump_net_map[bump] = &net;
    }
  }

  auto get_net_name = [&](const UnfoldedBump* bump) -> std::string {
    auto it = bump_net_map.find(bump);
    if (it != bump_net_map.end()) {
      return it->second->chip_net->getName();
    }
    return "defines no net";
  };

  dbMarkerCategory* cat = nullptr;
  for (const auto& conn : model->getConnections()) {
    if (!isValid(conn)) {
      continue;
    }
    if (!conn.top_region || !conn.bottom_region) {
      continue;
    }

    std::map<Point, const UnfoldedBump*> bot_bumps;
    for (const auto& bump : conn.bottom_region->bumps) {
      Point p(bump.global_position.x(), bump.global_position.y());
      bot_bumps[p] = &bump;
    }

    for (const auto& top_bump : conn.top_region->bumps) {
      Point p(top_bump.global_position.x(), top_bump.global_position.y());
      auto it = bot_bumps.find(p);
      if (it != bot_bumps.end()) {
        const UnfoldedBump* bot_bump = it->second;

        // Check logical connectivity
        auto top_net_it = bump_net_map.find(&top_bump);
        auto bot_net_it = bump_net_map.find(bot_bump);

        const UnfoldedNet* top_net
            = top_net_it != bump_net_map.end() ? top_net_it->second : nullptr;
        const UnfoldedNet* bot_net
            = bot_net_it != bump_net_map.end() ? bot_net_it->second : nullptr;

        if (top_net != bot_net) {
          if (!cat) {
            cat = dbMarkerCategory::createOrReplace(top_cat,
                                                    "Logical Connectivity");
          }
          if (auto* marker = dbMarker::create(cat)) {
            marker->addSource(top_bump.bump_inst);
            marker->addSource(bot_bump->bump_inst);
            marker->addShape(conn.top_region->cuboid.intersect(
                conn.bottom_region->cuboid));  // Mark overlap region

            std::string msg = fmt::format(
                "Bumps at ({}, {}) align physically but logical connectivity "
                "mismatch: Top bump {} vs Bottom bump {}",
                p.x(),
                p.y(),
                get_net_name(&top_bump),
                get_net_name(bot_bump));
            marker->setComment(msg);
            logger_->warn(utl::ODB, 208, msg);
          }
        }
      }
    }
  }
}

}  // namespace odb

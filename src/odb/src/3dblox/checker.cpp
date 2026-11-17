// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <ranges>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/PtrSetMap.h"
#include "odb/db.h"
#include "odb/dbObject.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "odb/geom.h"
#include "odb/geom_boost.h"  // IWYU pragma: keep
#include "spdlog/fmt/fmt.h"
#include "utl/Logger.h"
#include "utl/spatialIndex.h"
#include "utl/unionFind.h"

namespace odb {

namespace {

constexpr int kBumpMarkerHalfSize = 50;

// Local representation of an alignment-marker instance found while
// scanning an unfolded chip's leaf block. Holds the source dbInst plus
// its parent dbUnfoldedChipInst; bbox and orient are computed by applying
// the chip's world transform.
//
// Lives only inside checkAlignmentMarkers(). When STA-style
// dbUnfoldedInst is introduced, this becomes a filtered view over that
// table.
struct UnfoldedAlignmentMarker
{
  dbUnfoldedChipInst* parent_chip = nullptr;
  dbInst* inst = nullptr;

  Rect getBBox() const
  {
    Rect bbox = inst->getBBox()->getBox();
    parent_chip->getTransform().apply(bbox);
    return bbox;
  }

  dbOrientType getOrient() const
  {
    dbTransform inst_xform = inst->getTransform();
    dbTransform total = inst_xform;
    total.concat(parent_chip->getTransform());
    return total.getOrient();
  }
};

using AlignmentMarkerIndex
    = utl::SpatialIndex<Point, const UnfoldedAlignmentMarker*>;

std::vector<AlignmentMarkerIndex::Value> collectMarkersInRect(
    const std::vector<UnfoldedAlignmentMarker>& markers,
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
    const PtrMap<dbMaster, std::vector<dbAlignmentMarkerRule*>>&
        rules_by_master,
    const std::vector<AlignmentMarkerIndex::Value>& list_a,
    const std::vector<AlignmentMarkerIndex::Value>& list_b,
    dbUnfoldedChipInst* c_a,
    dbUnfoldedChipInst* c_b,
    std::unordered_set<const UnfoldedAlignmentMarker*>& matched_markers,
    const AlignmentViolationReporter& report)
{
  AlignmentMarkerIndex index_b(list_b);

  for (auto& [pa, marker] : list_a) {
    auto rules_it = rules_by_master.find(marker->inst->getMaster());
    if (rules_it == rules_by_master.end()) {
      continue;
    }
    const std::vector<dbAlignmentMarkerRule*>& rules = rules_it->second;

    int max_tolerance = 0;
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
        if (Point::manhattanDistance(a_marker->getBBox().center(),
                                     b_marker->getBBox().center())
            <= rule->getTolerance()) {
          counterparts.insert(candidate);
          if (!rule->getRelativeOrientations().empty()) {
            dbTransform rel_xform(a_marker->getOrient());
            rel_xform.invert();
            rel_xform.concat(dbTransform(b_marker->getOrient()));
            const dbOrientType relative_orient = rel_xform.getOrient();
            const auto& allowed = rule->getRelativeOrientations();
            if (std::ranges::find(allowed, relative_orient) == allowed.end()) {
              report(a_marker,
                     b_marker,
                     fmt::format(
                         "Alignment marker on {} has mismatched "
                         "relative orientation with counterpart on {} ({})",
                         b_marker->parent_chip->getName(),
                         a_marker->parent_chip->getName(),
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
                   marker->parent_chip->getName(),
                   counterparts.size(),
                   c_b->getName()));
      }
    }
    for (const auto& counterpart : counterparts) {
      matched_markers.insert(counterpart.second);
      index_b.remove(counterpart);
    }
  }
}

const char* sideToString(dbUnfoldedChipRegionInst::EffectiveSide side)
{
  switch (side) {
    case dbUnfoldedChipRegionInst::EffectiveSide::TOP:
      return "TOP";
    case dbUnfoldedChipRegionInst::EffectiveSide::BOTTOM:
      return "BOTTOM";
    case dbUnfoldedChipRegionInst::EffectiveSide::INTERNAL:
      return "INTERNAL";
    case dbUnfoldedChipRegionInst::EffectiveSide::INTERNAL_EXT:
      return "INTERNAL_EXT";
  }
  return "UNKNOWN";
}

// Whether the bump master's pin geometry includes a shape on the given
// layer. Used to verify a bump against its region's declared contact layer
// (dbChipRegion::getLayer()) -- the 3DBlox spec defines that layer as the
// one responsible for the bump's external connection (hybrid bond/micro
// bump), so a bump's own pins must actually land on it.
bool masterPinOnLayer(dbMaster* master, dbTechLayer* layer)
{
  for (dbMTerm* mterm : master->getMTerms()) {
    for (dbMPin* mpin : mterm->getMPins()) {
      for (dbBox* box : mpin->getGeometry()) {
        if (box->getTechLayer() == layer) {
          return true;
        }
      }
    }
  }
  return false;
}

enum class ConnectionStatus
{
  kValid,
  // The declared top/bot regions contradict the physical stackup: the top
  // region must face down and the bottom region must face up.
  kInvalidDirection,
  // The regions do not overlap in the XY plane.
  kInvalidXYOverlap,
  // The internal_ext regions do not overlap in Z.
  kInvalidZOverlap,
  // The gap between the mating surfaces does not match the connection
  // thickness.
  kInvalidThickness,
};

ConnectionStatus getConnectionStatus(dbUnfoldedChipConn* conn)
{
  dbUnfoldedChipRegionInst* top = conn->getTopRegion();
  dbUnfoldedChipRegionInst* bot = conn->getBottomRegion();
  if (!top || !bot) {
    return ConnectionStatus::kValid;
  }
  if (!top->getCuboid().xyIntersects(bot->getCuboid())) {
    return ConnectionStatus::kInvalidXYOverlap;
  }
  if (top->isInternalExt() || bot->isInternalExt()) {
    return std::max(top->getCuboid().zMin(), bot->getCuboid().zMin())
                   <= std::min(top->getCuboid().zMax(), bot->getCuboid().zMax())
               ? ConnectionStatus::kValid
               : ConnectionStatus::kInvalidZOverlap;
  }

  // The top region mates from the die above the connection, so its surface
  // must face down; the bottom region mates from below and must face up.
  if (!top->isBottom() || !bot->isTop()) {
    return ConnectionStatus::kInvalidDirection;
  }

  // A negative gap means the declared top surface lies below the bottom one.
  const int gap = top->getSurfaceZ() - bot->getSurfaceZ();
  if (gap != conn->getChipConn()->getThickness()) {
    return ConnectionStatus::kInvalidThickness;
  }
  return ConnectionStatus::kValid;
}

bool isValid(dbUnfoldedChipConn* conn)
{
  return getConnectionStatus(conn) == ConnectionStatus::kValid;
}

}  // namespace

Checker::Checker(utl::Logger* logger, dbDatabase* db) : logger_(logger), db_(db)
{
}

void Checker::check()
{
  dbChip* chip = db_->getChip();
  if (chip == nullptr) {
    return;
  }
  auto* top_cat = dbMarkerCategory::createOrReplace(chip, "3DBlox");
  checkLogicalConnectivity(top_cat);
  checkFloatingChips(top_cat);
  checkOverlappingChips(top_cat);
  checkInternalExtUsage(top_cat);
  checkConnectionRegions(top_cat);
  checkBumpPhysicalAlignment(top_cat);
  checkBumpLayer(top_cat);
  checkAlignmentMarkers(top_cat);
}

void Checker::computeBumpLayerMatches()
{
  // dbChipRegion::getLayer() is the contact layer responsible for the
  // region's external connections (hybrid bonds/micro bumps) per the
  // 3DBlox spec, so a bump's own pin geometry must land on it. Regions
  // without a declared layer are left unrecorded (unverifiable). The
  // result is session-only; nothing is persisted in ODB.
  for (dbChip* chip : db_->getChips()) {
    for (dbChipRegion* region : chip->getChipRegions()) {
      dbTechLayer* layer = region->getLayer();
      if (layer == nullptr) {
        continue;
      }
      for (dbChipBump* bump : region->getChipBumps()) {
        bump_layer_match_[bump]
            = masterPinOnLayer(bump->getInst()->getMaster(), layer);
      }
    }
  }
}

void Checker::checkFloatingChips(dbMarkerCategory* top_cat)
{
  std::vector<dbUnfoldedChipInst*> chips;
  for (dbUnfoldedChipInst* chip : db_->getUnfoldedChipInsts()) {
    chips.push_back(chip);
  }
  // Add one more node for "ground" (external world: package, PCB, ...)
  utl::UnionFind uf(chips.size() + 1);
  const size_t ground_node = chips.size();

  std::unordered_map<dbUnfoldedChipInst*, size_t> chip_map;
  for (size_t i = 0; i < chips.size(); ++i) {
    chip_map[chips[i]] = i;
  }

  for (dbUnfoldedChipConn* conn : db_->getUnfoldedChipConns()) {
    if (isValid(conn)) {
      // Case 1: Both regions exist - connect the two chips together
      if (conn->getTopRegion() && conn->getBottomRegion()) {
        auto it1 = chip_map.find(conn->getTopRegion()->getParentChip());
        auto it2 = chip_map.find(conn->getBottomRegion()->getParentChip());
        if (it1 != chip_map.end() && it2 != chip_map.end()) {
          uf.unite(it1->second, it2->second);
        }
      }
      // Case 2: Virtual connection (one region is null) - connect chip to
      // ground
      else if (conn->getTopRegion() || conn->getBottomRegion()) {
        dbUnfoldedChipRegionInst* region = conn->getTopRegion()
                                               ? conn->getTopRegion()
                                               : conn->getBottomRegion();
        auto it = chip_map.find(region->getParentChip());
        if (it != chip_map.end()) {
          uf.unite(it->second, ground_node);
        }
      }
    }
  }

  std::vector<std::vector<dbUnfoldedChipInst*>> groups(chips.size() + 1);
  for (size_t i = 0; i < chips.size(); ++i) {
    groups[uf.find(i)].push_back(chips[i]);
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
          marker->addShape(chip->getCuboid());
          marker->addSource(chip->getChipInstPath().back());
        }
        marker->setComment("Isolated chip set starting with "
                           + group[0]->getName());
      }
    }
  }
}

void Checker::checkOverlappingChips(dbMarkerCategory* top_cat)
{
  std::vector<dbUnfoldedChipInst*> chips;
  for (dbUnfoldedChipInst* chip : db_->getUnfoldedChipInsts()) {
    chips.push_back(chip);
  }
  std::ranges::sort(chips, [](dbUnfoldedChipInst* a, dbUnfoldedChipInst* b) {
    return a->getCuboid().xMin() < b->getCuboid().xMin();
  });

  std::vector<std::pair<dbUnfoldedChipInst*, dbUnfoldedChipInst*>> overlaps;
  for (size_t i = 0; i < chips.size(); ++i) {
    auto* c1 = chips[i];
    for (size_t j = i + 1; j < chips.size(); ++j) {
      auto* c2 = chips[j];
      if (c2->getCuboid().xMin() >= c1->getCuboid().xMax()) {
        break;
      }
      if (c1->getCuboid().overlaps(c2->getCuboid())) {
        overlaps.emplace_back(c1, c2);
      }
    }
  }

  if (!overlaps.empty()) {
    auto* cat = dbMarkerCategory::createOrReplace(top_cat, "Overlapping chips");
    logger_->warn(utl::ODB, 156, "Found {} overlapping chips", overlaps.size());
    for (const auto& [inst1, inst2] : overlaps) {
      if (auto* marker = dbMarker::create(cat)) {
        auto intersection = inst1->getCuboid().intersect(inst2->getCuboid());
        marker->addShape(intersection);
        marker->addSource(inst1->getChipInstPath().back());
        marker->addSource(inst2->getChipInstPath().back());
        marker->setComment(fmt::format(
            "Chips {} and {} overlap", inst1->getName(), inst2->getName()));
      }
    }
  }
}

void Checker::checkInternalExtUsage(dbMarkerCategory* top_cat)
{
  // The struct model maintained an isUsed bit on each region; in the
  // dbObject model we derive it here on demand by scanning connections.
  std::unordered_set<dbUnfoldedChipRegionInst*> used;
  for (dbUnfoldedChipConn* conn : db_->getUnfoldedChipConns()) {
    if (auto* r = conn->getTopRegion(); r && r->isInternalExt()) {
      used.insert(r);
    }
    if (auto* r = conn->getBottomRegion(); r && r->isInternalExt()) {
      used.insert(r);
    }
  }

  dbMarkerCategory* cat = nullptr;
  for (dbUnfoldedChipInst* chip : db_->getUnfoldedChipInsts()) {
    for (dbUnfoldedChipRegionInst* region : chip->getRegions()) {
      if (region->isInternalExt() && !used.contains(region)) {
        if (!cat) {
          cat = dbMarkerCategory::createOrReplace(top_cat,
                                                  "Unused internal_ext");
        }
        logger_->warn(utl::ODB,
                      464,
                      "Region {} is internal_ext but unused",
                      region->getChipRegionInst()->getChipRegion()->getName());
        if (auto* marker = dbMarker::create(cat)) {
          marker->addSource(region->getChipRegionInst());
          marker->addShape(region->getCuboid());
          marker->setComment(fmt::format(
              "Unused internal_ext region: {}",
              region->getChipRegionInst()->getChipRegion()->getName()));
        }
      }
    }
  }
}

void Checker::checkConnectionRegions(dbMarkerCategory* top_cat)
{
  auto describe = [](dbUnfoldedChipRegionInst* r, dbMarker* marker) {
    const Cuboid cuboid = r->getCuboid();
    marker->addSource(r->getChipRegionInst());
    marker->addShape(
        Rect(cuboid.xMin(), cuboid.yMin(), cuboid.xMax(), cuboid.yMax()));
    return fmt::format("{}/{} (faces {})",
                       r->getParentChip()->getName(),
                       r->getChipRegionInst()->getChipRegion()->getName(),
                       sideToString(r->getEffectiveSide()));
  };
  int count = 0;
  dbMarkerCategory* cat = nullptr;
  for (dbUnfoldedChipConn* conn : db_->getUnfoldedChipConns()) {
    const ConnectionStatus status = getConnectionStatus(conn);
    if (status != ConnectionStatus::kValid) {
      if (!cat) {
        cat = dbMarkerCategory::createOrReplace(top_cat, "Connection regions");
      }
      if (auto* marker = dbMarker::create(cat)) {
        marker->addSource(conn->getChipConn());
        const std::string conn_name = conn->getChipConn()->getName();
        const std::string top_desc = describe(conn->getTopRegion(), marker);
        const std::string bot_desc = describe(conn->getBottomRegion(), marker);
        std::string msg;
        switch (status) {
          case ConnectionStatus::kInvalidDirection:
            msg = fmt::format(
                "Ill-defined stacking direction for connection {}: top region "
                "{} must face BOTTOM and bottom region {} must face TOP",
                conn_name,
                top_desc,
                bot_desc);
            break;
          case ConnectionStatus::kInvalidXYOverlap:
            msg = fmt::format(
                "Invalid connection {}: {} and {} do not overlap in the XY "
                "plane",
                conn_name,
                top_desc,
                bot_desc);
            break;
          case ConnectionStatus::kInvalidZOverlap:
            msg = fmt::format(
                "Invalid connection {}: internal_ext regions {} and {} do not "
                "overlap in Z",
                conn_name,
                top_desc,
                bot_desc);
            break;
          case ConnectionStatus::kInvalidThickness:
            msg = fmt::format(
                "Invalid connection {}: gap {} between {} and {} does not "
                "match connection thickness {}",
                conn_name,
                conn->getTopRegion()->getSurfaceZ()
                    - conn->getBottomRegion()->getSurfaceZ(),
                top_desc,
                bot_desc,
                conn->getChipConn()->getThickness());
            break;
          case ConnectionStatus::kValid:
            break;
        }
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

void Checker::checkBumpPhysicalAlignment(dbMarkerCategory* top_cat)
{
  dbMarkerCategory* cat = nullptr;
  int violation_count = 0;
  for (dbUnfoldedChipInst* chip : db_->getUnfoldedChipInsts()) {
    for (dbUnfoldedChipRegionInst* region : chip->getRegions()) {
      for (dbUnfoldedChipBumpInst* bump : region->getBumps()) {
        const Point3D p = bump->getGlobalPosition();
        if (!region->getCuboid().getEnclosingRect().intersects(
                {p.x(), p.y()})) {
          violation_count++;
          if (!cat) {
            cat = dbMarkerCategory::createOrReplace(top_cat, "Bump Alignment");
          }
          // dbMarker::create returns nullptr once the category hits its
          // max_markers_ limit; skip the addSource/addShape/setComment
          // chain to avoid a null-deref crash in that case.
          if (auto* marker = dbMarker::create(cat)) {
            marker->addSource(bump->getChipBumpInst());
            marker->addShape(Rect(p.x() - kBumpMarkerHalfSize,
                                  p.y() - kBumpMarkerHalfSize,
                                  p.x() + kBumpMarkerHalfSize,
                                  p.y() + kBumpMarkerHalfSize));
            marker->setComment(fmt::format(
                "Bump is outside its parent region {}",
                region->getChipRegionInst()->getChipRegion()->getName()));
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

void Checker::checkNetConnectivity(dbMarkerCategory* top_cat)
{
}

void Checker::checkBumpLayer(dbMarkerCategory* top_cat)
{
  computeBumpLayerMatches();
  // Flag bumps whose pin geometry, per computeBumpLayerMatches(), does not
  // land on their region's declared contact layer.
  dbMarkerCategory* cat = nullptr;
  int violation_count = 0;
  for (dbUnfoldedChipBumpInst* bump : db_->getUnfoldedChipBumpInsts()) {
    dbChipBump* chip_bump = bump->getChipBumpInst()->getChipBump();
    const auto it = bump_layer_match_.find(chip_bump);
    if (it == bump_layer_match_.end() || it->second) {
      continue;
    }
    violation_count++;
    if (!cat) {
      cat = dbMarkerCategory::createOrReplace(top_cat, "Bump layer mismatch");
    }
    if (auto* marker = dbMarker::create(cat)) {
      const Point3D p = bump->getGlobalPosition();
      marker->addSource(bump->getChipBumpInst());
      marker->addShape(Rect(p.x() - kBumpMarkerHalfSize,
                            p.y() - kBumpMarkerHalfSize,
                            p.x() + kBumpMarkerHalfSize,
                            p.y() + kBumpMarkerHalfSize));
      dbChipRegion* chip_region = chip_bump->getChipRegion();
      marker->setComment(fmt::format(
          "Bump {} in region {} has no pin geometry on the region's "
          "declared contact layer {}",
          chip_bump->getInst()->getName(),
          chip_region->getName(),
          chip_region->getLayer()->getName()));
    }
  }
  if (violation_count > 0) {
    logger_->warn(utl::ODB,
                  554,
                  "Found {} bump(s) whose pin geometry does not match their "
                  "region's declared contact layer",
                  violation_count);
  }
}

void Checker::checkAlignmentMarkers(dbMarkerCategory* top_cat)
{
  // Build the per-master rule index. Local to this check; alignment-marker
  // rules are queried only here, so there's no point persisting it.
  PtrMap<dbMaster, std::vector<dbAlignmentMarkerRule*>> rules_by_master;
  for (dbAlignmentMarkerRule* rule : db_->getAlignmentMarkerRules()) {
    rules_by_master[rule->getMasterA()].push_back(rule);
    if (rule->getMasterB() != rule->getMasterA()) {
      rules_by_master[rule->getMasterB()].push_back(rule);
    }
  }
  if (rules_by_master.empty()) {
    return;
  }

  // Materialize the per-chip marker lists by walking each unfolded chip's
  // leaf block and filtering by master against the rule index.
  PtrMap<dbUnfoldedChipInst, std::vector<UnfoldedAlignmentMarker>> markers;
  for (dbUnfoldedChipInst* chip : db_->getUnfoldedChipInsts()) {
    std::vector<dbChipInst*> path = chip->getChipInstPath();
    if (path.empty()) {
      continue;
    }
    dbBlock* block = path.back()->getMasterChip()->getBlock();
    if (block == nullptr) {
      continue;
    }
    for (dbInst* inst : block->getInsts()) {
      if (rules_by_master.contains(inst->getMaster())) {
        markers[chip].push_back({.parent_chip = chip, .inst = inst});
      }
    }
  }

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
    if (marker == nullptr) {
      return;
    }
    // TODO: Add sources correctly
    marker->addShape(m->getBBox());
    if (other) {
      marker->addShape(other->getBBox());
    }
    marker->setComment(msg);
    violation_count++;
  };

  for (dbUnfoldedChipConn* conn : db_->getUnfoldedChipConns()) {
    if (!isValid(conn)) {
      continue;
    }
    dbUnfoldedChipRegionInst* ra = conn->getTopRegion();
    dbUnfoldedChipRegionInst* rb = conn->getBottomRegion();
    if (!ra || !rb) {
      continue;
    }
    dbUnfoldedChipInst* c_a = ra->getParentChip();
    dbUnfoldedChipInst* c_b = rb->getParentChip();
    if (c_a == c_b) {
      continue;
    }
    auto a_it = markers.find(c_a);
    auto b_it = markers.find(c_b);
    if (a_it == markers.end() && b_it == markers.end()) {
      continue;
    }

    const Rect overlap = ra->getCuboid().getEnclosingRect().intersect(
        rb->getCuboid().getEnclosingRect());
    auto list_a = (a_it != markers.end())
                      ? collectMarkersInRect(a_it->second, overlap)
                      : std::vector<AlignmentMarkerIndex::Value>{};
    auto list_b = (b_it != markers.end())
                      ? collectMarkersInRect(b_it->second, overlap)
                      : std::vector<AlignmentMarkerIndex::Value>{};
    if (list_a.empty() && list_b.empty()) {
      continue;
    }

    matchMarkersBetweenChips(
        rules_by_master, list_a, list_b, c_a, c_b, matched_markers, report);
  }

  for (const auto& [chip, chip_markers] : markers) {
    for (const auto& marker : chip_markers) {
      if (!matched_markers.contains(&marker)) {
        report(&marker,
               nullptr,
               fmt::format("Alignment marker on {} has no counterpart",
                           chip->getName()));
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

void Checker::checkLogicalConnectivity(dbMarkerCategory* top_cat)
{
  std::unordered_map<dbUnfoldedChipBumpInst*, dbUnfoldedChipNet*> bump_net_map;
  for (dbUnfoldedChipNet* net : db_->getUnfoldedChipNets()) {
    for (dbUnfoldedChipBumpInst* bump : net->getConnectedBumps()) {
      bump_net_map[bump] = net;
    }
  }

  auto get_net_name = [&](dbUnfoldedChipBumpInst* bump) -> std::string {
    auto it = bump_net_map.find(bump);
    if (it != bump_net_map.end()) {
      return it->second->getChipNet()->getName();
    }
    return "defines no net";
  };

  dbMarkerCategory* cat = nullptr;
  for (dbUnfoldedChipConn* conn : db_->getUnfoldedChipConns()) {
    if (!isValid(conn)) {
      continue;
    }
    dbUnfoldedChipRegionInst* top_region = conn->getTopRegion();
    dbUnfoldedChipRegionInst* bot_region = conn->getBottomRegion();
    if (!top_region || !bot_region) {
      continue;
    }

    std::map<Point, dbUnfoldedChipBumpInst*> bot_bumps;
    for (dbUnfoldedChipBumpInst* bump : bot_region->getBumps()) {
      const Point3D pos = bump->getGlobalPosition();
      bot_bumps[Point(pos.x(), pos.y())] = bump;
    }

    for (dbUnfoldedChipBumpInst* top_bump : top_region->getBumps()) {
      const Point3D top_pos = top_bump->getGlobalPosition();
      const Point p(top_pos.x(), top_pos.y());
      auto it = bot_bumps.find(p);
      if (it == bot_bumps.end()) {
        continue;
      }
      dbUnfoldedChipBumpInst* bot_bump = it->second;

      // Check logical connectivity
      auto top_net_it = bump_net_map.find(top_bump);
      auto bot_net_it = bump_net_map.find(bot_bump);

      dbUnfoldedChipNet* top_net
          = top_net_it != bump_net_map.end() ? top_net_it->second : nullptr;
      dbUnfoldedChipNet* bot_net
          = bot_net_it != bump_net_map.end() ? bot_net_it->second : nullptr;

      if (top_net != bot_net) {
        if (!cat) {
          cat = dbMarkerCategory::createOrReplace(top_cat,
                                                  "Logical Connectivity");
        }
        if (auto* marker = dbMarker::create(cat)) {
          marker->addSource(top_bump->getChipBumpInst());
          marker->addSource(bot_bump->getChipBumpInst());
          marker->addShape(top_region->getCuboid().intersect(
              bot_region->getCuboid()));  // Mark overlap region

          std::string msg = fmt::format(
              "Bumps at ({}, {}) align physically but logical connectivity "
              "mismatch: Top bump {} vs Bottom bump {}",
              p.x(),
              p.y(),
              get_net_name(top_bump),
              get_net_name(bot_bump));
          marker->setComment(msg);
          logger_->warn(utl::ODB, 208, msg);
        }
      }
    }
  }
}

}  // namespace odb

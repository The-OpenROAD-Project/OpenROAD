#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"
namespace odb {

// Compute the global (root-level) cuboid for a region instance by applying
// hierarchical transforms. Transform order (innermost to outermost):
//   1. Start with region's raw cuboid in master coordinates
//   2. Apply conn_path transforms (path from connection's chip to region)
//   3. Apply parent_path transforms (path from root to connection's chip)
// Both paths are traversed in reverse (leaf-to-root) to accumulate transforms
// correctly. This matches the transform accumulation in unfoldChip().
odb::Cuboid getGlobalCuboid(dbChipRegionInst* region_inst,
                            const std::vector<dbChipInst*>& conn_path,
                            const std::vector<dbChipInst*>& parent_path)
{
  odb::Cuboid cuboid = region_inst->getChipRegion()->getCuboid();

  for (auto inst : conn_path | std::views::reverse) {
    inst->getTransform().apply(cuboid);
  }
  for (auto inst : parent_path | std::views::reverse) {
    inst->getTransform().apply(cuboid);
  }

  return cuboid;
}

std::string UnfoldedChip::getName() const
{
  std::string name;
  int index = 0;
  for (auto chip_inst : chip_inst_path) {
    name += chip_inst->getName();
    if (index++ < chip_inst_path.size() - 1) {
      name += "/";
    }
  }
  return name;
}

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(odb::dbChip* chip)
{
  for (auto chip_inst : chip->getChipInsts()) {
    UnfoldedChip unfolded_chip;
    unfoldChip(chip_inst, unfolded_chip);
  }
  odb::dbMarkerCategory* category
      = odb::dbMarkerCategory::createOrReplace(chip, "3DBlox");

  checkConnectionRegions(chip, category);
  checkOverlappingChips(category);
  checkFloatingChips(category);
}

void Checker::unfoldChip(odb::dbChipInst* chip_inst,
                         UnfoldedChip& unfolded_chip)
{
  unfolded_chip.chip_inst_path.push_back(chip_inst);
  if (chip_inst->getMasterChip()->getChipType() == dbChip::ChipType::HIER) {
    for (auto chip_inst : chip_inst->getMasterChip()->getChipInsts()) {
      unfoldChip(chip_inst, unfolded_chip);
    }
  } else {
    // calculate the cuboid of the chip
    unfolded_chip.cuboid = chip_inst->getMasterChip()->getCuboid();
    for (auto chip_inst : unfolded_chip.chip_inst_path | std::views::reverse) {
      chip_inst->getTransform().apply(unfolded_chip.cuboid);
    }
    debugPrint(
        logger_,
        utl::ODB,
        "3dblox",
        1,
        "Unfolded chip: {} cuboid: ({}, {}, {}), ({}, {}, {})",
        unfolded_chip.getName(),
        unfolded_chip.cuboid.xMin() / chip_inst->getDb()->getDbuPerMicron(),
        unfolded_chip.cuboid.yMin() / chip_inst->getDb()->getDbuPerMicron(),
        unfolded_chip.cuboid.zMin() / chip_inst->getDb()->getDbuPerMicron(),
        unfolded_chip.cuboid.xMax() / chip_inst->getDb()->getDbuPerMicron(),
        unfolded_chip.cuboid.yMax() / chip_inst->getDb()->getDbuPerMicron(),
        unfolded_chip.cuboid.zMax() / chip_inst->getDb()->getDbuPerMicron());
    unfolded_chips_.push_back(unfolded_chip);
    // Cuboid is overwritten in each leaf - no restoration needed
  }
  unfolded_chip.chip_inst_path.pop_back();
}
void Checker::checkFloatingChips(odb::dbMarkerCategory* category)
{
  utl::UnionFind uf(unfolded_chips_.size());

  // Check all pairs for intersection and union them
  for (size_t i = 0; i < unfolded_chips_.size(); i++) {
    auto cuboid_i = unfolded_chips_[i].cuboid;
    for (size_t j = i + 1; j < unfolded_chips_.size(); j++) {
      auto cuboid_j = unfolded_chips_[j].cuboid;
      if (cuboid_i.intersects(cuboid_j)) {
        uf.unite(i, j);
      }
    }
  }

  // Group chips by their root parent
  std::map<int, std::vector<UnfoldedChip*>> sets;
  for (size_t i = 0; i < unfolded_chips_.size(); i++) {
    sets[uf.find(i)].push_back(&unfolded_chips_[i]);
  }

  if (sets.size() > 1) {
    // Convert to vector and sort by size
    std::vector<std::vector<UnfoldedChip*>> insts_sets;
    insts_sets.reserve(sets.size());
    for (auto& [root, chips] : sets) {
      insts_sets.emplace_back(chips);
    }

    std::ranges::sort(insts_sets,
                      [](const std::vector<UnfoldedChip*>& a,
                         const std::vector<UnfoldedChip*>& b) {
                        return a.size() > b.size();
                      });

    odb::dbMarkerCategory* floating_chips_category
        = odb::dbMarkerCategory::createOrReplace(category, "Floating chips");
    logger_->warn(
        utl::ODB, 151, "Found {} floating chip sets", insts_sets.size() - 1);

    // Create marker for each set except the first one (the biggest one)
    for (size_t i = 1; i < insts_sets.size(); i++) {
      auto& insts_set = insts_sets[i];
      odb::dbMarker* marker = odb::dbMarker::create(floating_chips_category);
      for (auto& inst : insts_set) {
        debugPrint(logger_,
                   utl::ODB,
                   "3dblox",
                   1,
                   "Floating chip: {}",
                   inst->getName());
        marker->addShape(Rect(inst->cuboid.xMin(),
                              inst->cuboid.yMin(),
                              inst->cuboid.xMax(),
                              inst->cuboid.yMax()));
        marker->addSource(inst->chip_inst_path.back());
      }
    }
  }
}

void Checker::checkOverlappingChips(odb::dbMarkerCategory* category)
{
  std::vector<std::pair<UnfoldedChip*, UnfoldedChip*>> overlaps;

  // Check all pairs of chip instances for overlaps
  for (size_t i = 0; i < unfolded_chips_.size(); i++) {
    auto cuboid_i = unfolded_chips_[i].cuboid;
    for (size_t j = i + 1; j < unfolded_chips_.size(); j++) {
      auto cuboid_j = unfolded_chips_[j].cuboid;
      if (cuboid_i.overlaps(cuboid_j)) {
        overlaps.emplace_back(&unfolded_chips_[i], &unfolded_chips_[j]);
      }
    }
  }

  if (!overlaps.empty()) {
    odb::dbMarkerCategory* overlapping_chips_category
        = odb::dbMarkerCategory::createOrReplace(category, "Overlapping chips");
    logger_->warn(utl::ODB, 156, "Found {} overlapping chips", overlaps.size());

    for (const auto& [inst1, inst2] : overlaps) {
      odb::dbMarker* marker = odb::dbMarker::create(overlapping_chips_category);

      // Compute the intersection region
      auto cuboid1 = inst1->cuboid;
      auto cuboid2 = inst2->cuboid;
      auto intersection = cuboid1.intersect(cuboid2);

      // Add the intersection as a shape (project to 2D for visualization)
      odb::Rect bbox(intersection.xMin(),
                     intersection.yMin(),
                     intersection.xMax(),
                     intersection.yMax());
      marker->addShape(bbox);

      // Add both chip instances as sources
      marker->addSource(inst1->chip_inst_path.back());
      marker->addSource(inst2->chip_inst_path.back());

      // Add a comment describing the overlap
      std::string comment = "Chips " + inst1->getName() + " and "
                            + inst2->getName() + " overlap";
      debugPrint(logger_,
                 utl::ODB,
                 "3dblox",
                 1,
                 "Overlapping chips: {} and {}",
                 inst1->getName(),
                 inst2->getName());
      marker->setComment(comment);
    }
  }
}

void Checker::checkConnectionRegions(odb::dbChip* chip,
                                     odb::dbMarkerCategory* category)
{
  odb::dbMarkerCategory* conn_category
      = odb::dbMarkerCategory::createOrReplace(category, "Connected regions");

  // Pass errors by reference for recursive accumulation across hierarchy.
  // Note: Current implementation is O(connections * hierarchy_depth).
  // Future optimization should consider spatial indexing (R-tree) for region
  // intersection queries.
  int errors = 0;
  std::vector<dbChipInst*> path;
  checkConnectionRegionsRecursive(chip, conn_category, path, errors);

  if (errors > 0) {
    logger_->warn(
        utl::ODB, 206, "Found {} non-intersecting connections", errors);
  }
}

void Checker::checkConnectionRegionsRecursive(odb::dbChip* chip,
                                              odb::dbMarkerCategory* category,
                                              std::vector<dbChipInst*>& path,
                                              int& errors)
{
  for (auto conn : chip->getChipConns()) {
    auto top_region_inst = conn->getTopRegion();
    auto bottom_region_inst = conn->getBottomRegion();

    if (!top_region_inst || !bottom_region_inst) {
      logger_->warn(utl::ODB,
                    404,
                    "Connection {} has missing regions (top: {}, bottom: {})",
                    conn->getName(),
                    top_region_inst ? "found" : "null",
                    bottom_region_inst ? "found" : "null");
      continue;
    }

    odb::Cuboid top_cuboid
        = getGlobalCuboid(top_region_inst, conn->getTopRegionPath(), path);
    odb::Cuboid bottom_cuboid = getGlobalCuboid(
        bottom_region_inst, conn->getBottomRegionPath(), path);

    // Bloat each region by half the connection thickness.
    // Total tolerance = thickness, representing the maximum allowed gap
    // between the two surfaces for them to be considered "connected".
    top_cuboid.bloat(conn->getThickness() / 2.0, top_cuboid);
    bottom_cuboid.bloat(conn->getThickness() / 2.0, bottom_cuboid);

    if (!top_cuboid.intersects(bottom_cuboid)) {
      errors++;
      odb::dbMarker* marker = odb::dbMarker::create(category);
      odb::Rect bbox(top_cuboid.xMin(),
                     top_cuboid.yMin(),
                     top_cuboid.xMax(),
                     top_cuboid.yMax());
      marker->addShape(bbox);
      bbox = odb::Rect(bottom_cuboid.xMin(),
                       bottom_cuboid.yMin(),
                       bottom_cuboid.xMax(),
                       bottom_cuboid.yMax());
      marker->addShape(bbox);

      marker->addSource(conn);

      std::string comment
          = "Connection " + conn->getName() + " regions do not intersect";
      marker->setComment(comment);
      debugPrint(logger_,
                 utl::ODB,
                 "3dblox",
                 1,
                 "Connection {} regions do not intersect (top: "
                 "[{},{},{}]-[{},{},{}], bottom: [{},{},{}]-[{},{},{}])",
                 conn->getName(),
                 top_cuboid.xMin(),
                 top_cuboid.yMin(),
                 top_cuboid.zMin(),
                 top_cuboid.xMax(),
                 top_cuboid.yMax(),
                 top_cuboid.zMax(),
                 bottom_cuboid.xMin(),
                 bottom_cuboid.yMin(),
                 bottom_cuboid.zMin(),
                 bottom_cuboid.xMax(),
                 bottom_cuboid.yMax(),
                 bottom_cuboid.zMax());
    }
  }

  // Recurse into sub-chips
  for (auto inst : chip->getChipInsts()) {
    path.push_back(inst);
    checkConnectionRegionsRecursive(
        inst->getMasterChip(), category, path, errors);
    path.pop_back();
  }
}

}  // namespace odb

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(odb::dbChip* chip)
{
  odb::dbMarkerCategory* category
      = odb::dbMarkerCategory::createOrReplace(chip, "3DBlox");
  checkFloatingChips(chip, category);
  checkOverlappingChips(chip, category);
  // checkConnectionRegions(chip, category);
}

void Checker::checkFloatingChips(odb::dbChip* chip,
                                 odb::dbMarkerCategory* category)
{
  std::vector<dbChipInst*> chip_insts;
  for (auto chip_inst : chip->getChipInsts()) {
    chip_insts.push_back(chip_inst);
  }

  if (chip_insts.empty()) {
    return;
  }

  // Union-Find data structure
  std::vector<int> parent(chip_insts.size());
  std::vector<int> rank(chip_insts.size(), 0);

  // Initialize: each chip is its own parent
  for (size_t i = 0; i < chip_insts.size(); i++) {
    parent[i] = i;
  }

  // Find with path compression
  std::function<int(int)> find = [&](int x) {
    if (parent[x] != x) {
      parent[x] = find(parent[x]);
    }
    return parent[x];
  };

  // Union by rank
  auto unite = [&](int x, int y) {
    int px = find(x);
    int py = find(y);
    if (px == py) {
      return;
    }
    if (rank[px] < rank[py]) {
      parent[px] = py;
    } else if (rank[px] > rank[py]) {
      parent[py] = px;
    } else {
      parent[py] = px;
      rank[px]++;
    }
  };

  // Check all pairs for intersection and union them
  for (size_t i = 0; i < chip_insts.size(); i++) {
    auto cuboid_i = chip_insts[i]->getCuboid();
    for (size_t j = i + 1; j < chip_insts.size(); j++) {
      auto cuboid_j = chip_insts[j]->getCuboid();
      if (cuboid_i.intersects(cuboid_j)) {
        unite(i, j);
      }
    }
  }

  // Group chips by their root parent
  std::map<int, std::vector<dbChipInst*>> sets;
  for (size_t i = 0; i < chip_insts.size(); i++) {
    sets[find(i)].push_back(chip_insts[i]);
  }

  if (sets.size() > 1) {
    // Convert to vector and sort by size
    std::vector<std::vector<dbChipInst*>> insts_sets;
    insts_sets.reserve(sets.size());
    for (auto& [root, chips] : sets) {
      insts_sets.emplace_back(chips);
    }

    std::sort(
        insts_sets.begin(),
        insts_sets.end(),
        [](const std::vector<dbChipInst*>& a,
           const std::vector<dbChipInst*>& b) { return a.size() > b.size(); });

    odb::dbMarkerCategory* floating_chips_category
        = odb::dbMarkerCategory::createOrReplace(category, "Floating chips");
    logger_->warn(
        utl::ODB, 151, "Found {} floating chip sets", insts_sets.size() - 1);

    // Create marker for each set except the first one (the biggest one)
    for (size_t i = 1; i < insts_sets.size(); i++) {
      auto& insts_set = insts_sets[i];
      odb::dbMarker* marker = odb::dbMarker::create(floating_chips_category);
      for (auto& inst : insts_set) {
        logger_->report("Floating chip: {}", inst->getName());
        marker->addShape(inst->getBBox());
        marker->addSource(inst);
      }
    }
  }
}

void Checker::checkOverlappingChips(odb::dbChip* chip,
                                    odb::dbMarkerCategory* category)
{
  std::vector<dbChipInst*> chip_insts;
  for (auto chip_inst : chip->getChipInsts()) {
    chip_insts.push_back(chip_inst);
  }

  std::vector<std::pair<dbChipInst*, dbChipInst*>> overlaps;

  // Check all pairs of chip instances for overlaps
  for (size_t i = 0; i < chip_insts.size(); i++) {
    auto cuboid_i = chip_insts[i]->getCuboid();
    for (size_t j = i + 1; j < chip_insts.size(); j++) {
      auto cuboid_j = chip_insts[j]->getCuboid();
      if (cuboid_i.overlaps(cuboid_j)) {
        overlaps.emplace_back(chip_insts[i], chip_insts[j]);
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
      auto cuboid1 = inst1->getCuboid();
      auto cuboid2 = inst2->getCuboid();
      auto intersection = cuboid1.intersect(cuboid2);

      // Add the intersection as a shape (project to 2D for visualization)
      odb::Rect bbox(intersection.xMin(),
                     intersection.yMin(),
                     intersection.xMax(),
                     intersection.yMax());
      marker->addShape(bbox);

      // Add both chip instances as sources
      marker->addSource(inst1);
      marker->addSource(inst2);

      // Add a comment describing the overlap
      std::string comment = "Chips " + inst1->getName() + " and "
                            + inst2->getName() + " overlap";
      marker->setComment(comment);
    }
  }
}

}  // namespace odb
#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"
namespace odb {

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
    unfoldChip(chip_inst, UnfoldedChip());
  }
  odb::dbMarkerCategory* category
      = odb::dbMarkerCategory::createOrReplace(chip, "3DBlox");
  checkFloatingChips(category);
  checkOverlappingChips(category);
  // checkConnectionRegions(chip, category);
}

void Checker::unfoldChip(odb::dbChipInst* chip_inst, UnfoldedChip unfolded_chip)
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
  }
}
void Checker::checkFloatingChips(odb::dbMarkerCategory* category)
{
  // Union-Find data structure
  std::vector<int> parent(unfolded_chips_.size());
  std::vector<int> rank(unfolded_chips_.size(), 0);

  // Initialize: each chip is its own parent
  for (size_t i = 0; i < unfolded_chips_.size(); i++) {
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
  for (size_t i = 0; i < unfolded_chips_.size(); i++) {
    auto cuboid_i = unfolded_chips_[i].cuboid;
    for (size_t j = i + 1; j < unfolded_chips_.size(); j++) {
      auto cuboid_j = unfolded_chips_[j].cuboid;
      if (cuboid_i.intersects(cuboid_j)) {
        unite(i, j);
      }
    }
  }

  // Group chips by their root parent
  std::map<int, std::vector<UnfoldedChip*>> sets;
  for (size_t i = 0; i < unfolded_chips_.size(); i++) {
    sets[find(i)].push_back(&unfolded_chips_[i]);
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

}  // namespace odb
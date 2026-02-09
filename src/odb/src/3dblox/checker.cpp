// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "checker.h"

#include <algorithm>
#include <cstddef>
#include <map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "unfoldedModel.h"
#include "utl/Logger.h"
#include "utl/unionFind.h"

namespace odb {

Checker::Checker(utl::Logger* logger) : logger_(logger)
{
}

void Checker::check(odb::dbChip* chip)
{
  UnfoldedModel model(logger_, chip);

  odb::dbMarkerCategory* category
      = odb::dbMarkerCategory::createOrReplace(chip, "3DBlox");
  checkFloatingChips(model, category);
  checkOverlappingChips(model, category);
}

void Checker::checkFloatingChips(const UnfoldedModel& model,
                                 odb::dbMarkerCategory* category)
{
  const auto& chips = model.getChips();
  utl::UnionFind union_find(chips.size());

  for (size_t i = 0; i < chips.size(); i++) {
    auto cuboid_i = chips[i].cuboid;
    for (size_t j = i + 1; j < chips.size(); j++) {
      auto cuboid_j = chips[j].cuboid;
      if (cuboid_i.intersects(cuboid_j)) {
        union_find.unite(i, j);
      }
    }
  }

  std::map<int, std::vector<const UnfoldedChip*>> sets;
  for (size_t i = 0; i < chips.size(); i++) {
    sets[union_find.find(i)].push_back(&chips[i]);
  }

  if (sets.size() > 1) {
    std::vector<std::vector<const UnfoldedChip*>> insts_sets;
    insts_sets.reserve(sets.size());
    for (auto& [root, chips_list] : sets) {
      insts_sets.emplace_back(chips_list);
    }

    std::ranges::sort(insts_sets,
                      [](const std::vector<const UnfoldedChip*>& a,
                         const std::vector<const UnfoldedChip*>& b) -> bool {
                        return a.size() > b.size();
                      });

    odb::dbMarkerCategory* floating_chips_category
        = odb::dbMarkerCategory::createOrReplace(category, "Floating chips");
    logger_->warn(utl::ODB,
                  151,
                  "Found {} floating chip sets",
                  (int) insts_sets.size() - 1);

    for (size_t i = 1; i < insts_sets.size(); i++) {
      auto& insts_set = insts_sets[i];
      odb::dbMarker* marker = odb::dbMarker::create(floating_chips_category);
      for (auto* inst : insts_set) {
        marker->addShape(Rect(inst->cuboid.xMin(),
                              inst->cuboid.yMin(),
                              inst->cuboid.xMax(),
                              inst->cuboid.yMax()));
        marker->addSource(inst->chip_inst_path.back());
      }
    }
  }
}

void Checker::checkOverlappingChips(const UnfoldedModel& model,
                                    odb::dbMarkerCategory* category)
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
    odb::dbMarkerCategory* overlapping_chips_category
        = odb::dbMarkerCategory::createOrReplace(category, "Overlapping chips");
    logger_->warn(
        utl::ODB, 156, "Found {} overlapping chips", (int) overlaps.size());

    for (const auto& [inst1, inst2] : overlaps) {
      odb::dbMarker* marker = odb::dbMarker::create(overlapping_chips_category);

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

void Checker::checkConnectionRegions(const UnfoldedModel& model,
                                     dbChip* chip,
                                     dbMarkerCategory* category)
{
}

void Checker::checkBumpPhysicalAlignment(const UnfoldedModel& model,
                                         dbMarkerCategory* category)
{
}

void Checker::checkNetConnectivity(const UnfoldedModel& model,
                                   dbChip* chip,
                                   dbMarkerCategory* category)
{
}

bool Checker::isOverlapFullyInConnections(const UnfoldedChip* chip1,
                                          const UnfoldedChip* chip2,
                                          const Cuboid& overlap) const
{
  return false;
}

}  // namespace odb

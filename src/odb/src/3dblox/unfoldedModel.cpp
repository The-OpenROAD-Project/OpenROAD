// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "unfoldedModel.h"

#include <ranges>
#include <string>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "utl/Logger.h"

namespace odb {

int UnfoldedRegionFull::getSurfaceZ() const
{
  return 0;
}

bool UnfoldedRegionFull::isFacingUp() const
{
  return false;
}

bool UnfoldedRegionFull::isFacingDown() const
{
  return false;
}

bool UnfoldedConnection::isValid() const
{
  return false;
}

std::vector<UnfoldedBump*> UnfoldedNet::getDisconnectedBumps(
    int bump_pitch_tolerance) const
{
  return {};
}

std::string UnfoldedChip::getName() const
{
  std::string name;
  char delimiter = '/';
  if (!chip_inst_path.empty()) {
    dbBlock* block = chip_inst_path[0]->getParentChip()->getBlock();
    if (block) {
      char d = block->getHierarchyDelimiter();
      if (d != 0) {
        delimiter = d;
      }
    }
  }
  for (auto* chip_inst : chip_inst_path) {
    if (!name.empty()) {
      name += delimiter;
    }
    name += chip_inst->getName();
  }
  return name;
}

std::string UnfoldedChip::getPathKey() const
{
  return getName();
}

UnfoldedModel::UnfoldedModel(utl::Logger* logger) : logger_(logger)
{
}

void UnfoldedModel::build(dbChip* chip)
{
  for (dbChipInst* chip_inst : chip->getChipInsts()) {
    UnfoldedChip unfolded_chip;
    unfoldChip(chip_inst, unfolded_chip);
  }
  unfoldConnections(chip);
  unfoldNets(chip);
}

void UnfoldedModel::unfoldChip(dbChipInst* chip_inst,
                               UnfoldedChip& unfolded_chip)
{
  unfolded_chip.chip_inst_path.push_back(chip_inst);

  if (chip_inst->getMasterChip()->getChipType() == dbChip::ChipType::HIER) {
    for (auto sub_inst : chip_inst->getMasterChip()->getChipInsts()) {
      unfoldChip(sub_inst, unfolded_chip);
    }
  } else {
    // Original logic: Leaf chip cuboid calculation
    unfolded_chip.cuboid = chip_inst->getMasterChip()->getCuboid();
    for (auto inst : unfolded_chip.chip_inst_path | std::views::reverse) {
      inst->getTransform().apply(unfolded_chip.cuboid);
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
    chip_path_map_[unfolded_chip.getPathKey()] = &unfolded_chips_.back();
  }

  unfolded_chip.chip_inst_path.pop_back();
}

void UnfoldedModel::unfoldBumps(UnfoldedRegionFull& uf_region,
                                const std::vector<dbChipInst*>& path)
{
}

UnfoldedChip* UnfoldedModel::findUnfoldedChip(
    const std::vector<dbChipInst*>& path)
{
  return nullptr;
}

UnfoldedRegionFull* UnfoldedModel::findUnfoldedRegion(
    UnfoldedChip* chip,
    dbChipRegionInst* region_inst)
{
  return nullptr;
}

void UnfoldedModel::unfoldConnections(dbChip* chip)
{
}

void UnfoldedModel::unfoldConnectionsRecursive(
    dbChip* chip,
    const std::vector<dbChipInst*>& parent_path)
{
}

void UnfoldedModel::unfoldNets(dbChip* chip)
{
}

}  // namespace odb

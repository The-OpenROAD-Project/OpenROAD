// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "odb/unfoldedModel.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {

namespace {

std::vector<dbChipInst*> concatPath(const std::vector<dbChipInst*>& head,
                                    const std::vector<dbChipInst*>& tail)
{
  if (tail.empty()) {
    return head;
  }
  std::vector<dbChipInst*> full = head;
  full.insert(full.end(), tail.begin(), tail.end());
  return full;
}

std::string getFullPathName(const std::vector<dbChipInst*>& path)
{
  std::string name;
  for (auto* p : path) {
    if (!name.empty()) {
      name += '/';
    }
    name += p->getName();
  }
  return name;
}

UnfoldedRegionSide mirrorSide(UnfoldedRegionSide side)
{
  if (side == UnfoldedRegionSide::TOP) {
    return UnfoldedRegionSide::BOTTOM;
  }
  if (side == UnfoldedRegionSide::BOTTOM) {
    return UnfoldedRegionSide::TOP;
  }
  return side;
}

}  // namespace

Cuboid UnfoldedChip::getCuboid() const
{
  Cuboid cuboid = chip_inst_path.back()->getMasterChip()->getCuboid();
  transform.apply(cuboid);
  return cuboid;
}

std::string UnfoldedChip::getFullName() const
{
  return getFullPathName(chip_inst_path);
}

Cuboid UnfoldedRegion::getCuboid() const
{
  Cuboid cuboid = region_inst->getChipRegion()->getCuboid();
  parent_chip->transform.apply(cuboid);
  return cuboid;
}

int UnfoldedRegion::getSurfaceZ() const
{
  if (isTop()) {
    return getCuboid().zMax();
  }
  if (isBottom()) {
    return getCuboid().zMin();
  }
  return getCuboid().zCenter();
}

Point3D UnfoldedBump::getGlobalPosition() const
{
  Point global_xy = bump_inst->getChipBump()->getInst()->getLocation();
  parent_region->parent_chip->transform.apply(global_xy);
  return Point3D(global_xy.x(), global_xy.y(), parent_region->getSurfaceZ());
}

UnfoldedRegion* UnfoldedChip::findUnfoldedRegion(dbChipRegionInst* inst)
{
  if (inst == nullptr) {
    return nullptr;
  }
  auto comp
      = [](const UnfoldedRegion& uf_region, const dbChipRegionInst* inst) {
          return uf_region.region_inst->getId() < inst->getId();
        };

  auto it = std::lower_bound(regions.begin(), regions.end(), inst, comp);
  if (it != regions.end() && (*it).region_inst == inst) {
    return &(*it);
  }
  return nullptr;
}

UnfoldedBump* UnfoldedChip::findUnfoldedBump(dbChipBumpInst* bump_inst)
{
  if (bump_inst == nullptr) {
    return nullptr;
  }
  auto region = findUnfoldedRegion(bump_inst->getChipRegionInst());
  if (region == nullptr) {
    return nullptr;
  }
  auto comp = [](const UnfoldedBump& uf_bump, const dbChipBumpInst* bump_inst) {
    return uf_bump.bump_inst->getId() < bump_inst->getId();
  };
  auto it = std::lower_bound(
      region->bumps.begin(), region->bumps.end(), bump_inst, comp);
  if (it != region->bumps.end() && (*it).bump_inst == bump_inst) {
    return &(*it);
  }
  return nullptr;
}

UnfoldedModel::UnfoldedModel(utl::Logger* logger, dbChip* chip)
    : logger_(logger)
{
  std::vector<dbChipInst*> path;
  for (dbChipInst* inst : chip->getChipInsts()) {
    buildUnfoldedChip(inst, path, dbTransform());
  }
  unfoldConnections(chip, {});
  unfoldNets(chip, {});
  attachObservers(chip);
}

void UnfoldedModel::attachObservers(dbChip* chip)
{
  if (!chip || !observed_chips_.insert(chip).second) {
    return;
  }

  chip_observers_.emplace_back(this);
  chip_observers_.back().addOwner(chip);

  if (dbBlock* block = chip->getBlock()) {
    if (observed_blocks_.insert(block).second) {
      block_observers_.emplace_back(this);
      block_observers_.back().addOwner(block);
    }
  }

  for (dbChipInst* inst : chip->getChipInsts()) {
    attachObservers(inst->getMasterChip());
  }
}

UnfoldedChip* UnfoldedModel::buildUnfoldedChip(dbChipInst* inst,
                                               std::vector<dbChipInst*>& path,
                                               const dbTransform& parent_xform)
{
  dbChip* master = inst->getMasterChip();
  path.push_back(inst);

  const dbTransform inst_xform = inst->getTransform();
  dbTransform total = inst_xform;
  total.concat(parent_xform);

  if (master->getChipType() == dbChip::ChipType::HIER) {
    for (auto* sub : master->getChipInsts()) {
      buildUnfoldedChip(sub, path, total);
    }

    unfoldConnections(master, path);
    unfoldNets(master, path);
    path.pop_back();
    return nullptr;
  }

  std::unique_ptr<UnfoldedChip> uf_chip = std::make_unique<UnfoldedChip>();
  auto uf_chip_ptr = uf_chip.get();
  uf_chip->chip_inst_path = path;
  uf_chip->transform = total;

  unfolded_chips_.push_back(std::move(uf_chip));
  unfoldRegions(uf_chip_ptr, inst);

  chip_map_[uf_chip_ptr->getFullName()] = uf_chip_ptr;

  path.pop_back();
  return uf_chip_ptr;
}

void UnfoldedModel::unfoldRegions(UnfoldedChip* uf_chip, dbChipInst* inst)
{
  std::vector<dbChipRegionInst*> region_insts;
  for (auto* region_inst : inst->getRegions()) {
    region_insts.push_back(region_inst);
  }
  std::sort(region_insts.begin(),
            region_insts.end(),
            [](const dbChipRegionInst* a, const dbChipRegionInst* b) {
              return a->getId() < b->getId();
            });

  for (auto* region_inst : region_insts) {
    auto region = region_inst->getChipRegion();

    UnfoldedRegionSide side = UnfoldedRegionSide::INTERNAL;
    switch (region->getSide()) {
      case dbChipRegion::Side::FRONT:
        side = UnfoldedRegionSide::TOP;
        break;
      case dbChipRegion::Side::BACK:
        side = UnfoldedRegionSide::BOTTOM;
        break;
      case dbChipRegion::Side::INTERNAL:
        side = UnfoldedRegionSide::INTERNAL;
        break;
      case dbChipRegion::Side::INTERNAL_EXT:
        side = UnfoldedRegionSide::INTERNAL_EXT;
        break;
    }

    if (uf_chip->transform.isMirrorZ()) {
      side = mirrorSide(side);
    }

    UnfoldedRegion uf_region;
    uf_region.region_inst = region_inst;
    uf_region.effective_side = side;
    uf_region.parent_chip = uf_chip;

    uf_chip->regions.push_back(std::move(uf_region));
    unfoldBumps(uf_chip->regions.back(), uf_chip->transform);
  }
}

void UnfoldedModel::unfoldBumps(UnfoldedRegion& uf_region,
                                const dbTransform& transform)
{
  std::vector<dbChipBumpInst*> bump_insts;
  for (auto* bump_inst : uf_region.region_inst->getChipBumpInsts()) {
    bump_insts.push_back(bump_inst);
  }
  std::sort(bump_insts.begin(),
            bump_insts.end(),
            [](const dbChipBumpInst* a, const dbChipBumpInst* b) {
              return a->getId() < b->getId();
            });
  for (auto* bump_inst : bump_insts) {
    dbChipBump* bump = bump_inst->getChipBump();
    if (bump->getInst()) {
      uf_region.bumps.push_back(
          UnfoldedBump{.bump_inst = bump_inst, .parent_region = &uf_region});
    }
  }
}

UnfoldedChip* UnfoldedModel::findUnfoldedChip(const std::string& path)
{
  auto it = chip_map_.find(path);
  return it != chip_map_.end() ? it->second : nullptr;
}

UnfoldedChip* UnfoldedModel::findUnfoldedChip(
    const std::vector<dbChipInst*>& path)
{
  return findUnfoldedChip(getFullPathName(path));
}

void UnfoldedModel::unfoldConnections(
    dbChip* chip,
    const std::vector<dbChipInst*>& parent_path)
{
  for (auto* conn : chip->getChipConns()) {
    UnfoldedChip* top_chip
        = findUnfoldedChip(concatPath(parent_path, conn->getTopRegionPath()));
    UnfoldedRegion* top_region(nullptr);
    if (top_chip) {
      top_region = top_chip->findUnfoldedRegion(conn->getTopRegion());
    }
    UnfoldedChip* bot_chip = findUnfoldedChip(
        concatPath(parent_path, conn->getBottomRegionPath()));
    UnfoldedRegion* bot_region(nullptr);
    if (bot_chip) {
      bot_region = bot_chip->findUnfoldedRegion(conn->getBottomRegion());
    }
    if (top_region || bot_region) {
      UnfoldedConnection uf_conn{.connection = conn,
                                 .top_region = top_region,
                                 .bottom_region = bot_region};
      if (top_region && top_region->isInternalExt()) {
        top_region->isUsed = true;
      }
      if (bot_region && bot_region->isInternalExt()) {
        bot_region->isUsed = true;
      }
      unfolded_connections_.push_back(uf_conn);
    }
  }
}

void UnfoldedModel::unfoldNets(dbChip* chip,
                               const std::vector<dbChipInst*>& parent_path)
{
  for (auto* net : chip->getChipNets()) {
    UnfoldedNet uf_net;
    uf_net.chip_net = net;
    for (uint32_t i = 0; i < net->getNumBumpInsts(); i++) {
      std::vector<dbChipInst*> rel_path;
      dbChipBumpInst* b_inst = net->getBumpInst(i, rel_path);
      UnfoldedChip* chip = findUnfoldedChip(concatPath(parent_path, rel_path));
      if (chip) {
        UnfoldedBump* bump = chip->findUnfoldedBump(b_inst);
        if (bump) {
          uf_net.connected_bumps.push_back(bump);
        }
      }
    }
    unfolded_nets_.push_back(std::move(uf_net));
  }
}

}  // namespace odb

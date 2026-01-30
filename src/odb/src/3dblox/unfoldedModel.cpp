// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#include "unfoldedModel.h"

#include <algorithm>
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

namespace {

std::vector<odb::dbChipInst*> concatPath(
    const std::vector<odb::dbChipInst*>& head,
    const std::vector<odb::dbChipInst*>& tail)
{
  if (tail.empty()) {
    return head;
  }
  std::vector<odb::dbChipInst*> full = head;
  full.insert(full.end(), tail.begin(), tail.end());
  return full;
}

std::string getFullPathName(const std::vector<odb::dbChipInst*>& path)
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

odb::dbChipRegion::Side mirrorSide(odb::dbChipRegion::Side side)
{
  if (side == odb::dbChipRegion::Side::FRONT) {
    return odb::dbChipRegion::Side::BACK;
  }
  if (side == odb::dbChipRegion::Side::BACK) {
    return odb::dbChipRegion::Side::FRONT;
  }
  return side;
}

}  // namespace

namespace odb {

int UnfoldedRegion::getSurfaceZ() const
{
  if (isFront()) {
    return cuboid.zMax();
  }
  if (isBack()) {
    return cuboid.zMin();
  }
  return cuboid.zCenter();
}

bool UnfoldedChip::isParentOf(const UnfoldedChip* other) const
{
  if (chip_inst_path.size() >= other->chip_inst_path.size()) {
    return false;
  }
  return std::equal(chip_inst_path.begin(),
                    chip_inst_path.end(),
                    other->chip_inst_path.begin());
}

UnfoldedModel::UnfoldedModel(utl::Logger* logger, dbChip* chip)
    : logger_(logger)
{
  std::vector<dbChipInst*> path;
  for (dbChipInst* inst : chip->getChipInsts()) {
    Cuboid local;
    buildUnfoldedChip(inst, path, dbTransform(), local);
  }
  unfoldConnections(chip, {});
  unfoldNets(chip, {});
}

UnfoldedChip* UnfoldedModel::buildUnfoldedChip(dbChipInst* inst,
                                               std::vector<dbChipInst*>& path,
                                               const dbTransform& parent_xform,
                                               Cuboid& local)
{
  dbChip* master = inst->getMasterChip();
  path.push_back(inst);

  const dbTransform inst_xform = inst->getTransform();
  dbTransform total = inst_xform;
  total.concat(parent_xform);

  UnfoldedChip uf_chip;
  uf_chip.name = getFullPathName(path);
  uf_chip.chip_inst_path = path;
  uf_chip.transform = total;

  if (master->getChipType() == dbChip::ChipType::HIER) {
    uf_chip.cuboid.mergeInit();
    for (auto* sub : master->getChipInsts()) {
      Cuboid sub_local;
      buildUnfoldedChip(sub, path, total, sub_local);
      uf_chip.cuboid.merge(sub_local);
    }
  } else {
    uf_chip.cuboid = master->getCuboid();
  }

  // Calculate cuboid in parent space for hierarchical merging
  local = uf_chip.cuboid;
  inst_xform.apply(local);

  // Transform cuboid to global space
  uf_chip.transform.apply(uf_chip.cuboid);
  unfoldRegions(uf_chip, inst, uf_chip.transform);

  unfolded_chips_.push_back(std::move(uf_chip));
  registerUnfoldedChip(unfolded_chips_.back());

  unfoldConnections(master, path);
  unfoldNets(master, path);

  path.pop_back();
  return &unfolded_chips_.back();
}

void UnfoldedModel::registerUnfoldedChip(UnfoldedChip& chip)
{
  for (auto& region : chip.regions) {
    region.parent_chip = &chip;
    chip.region_map[region.region_inst] = &region;
    for (auto& bump : region.bumps) {
      bump.parent_region = &region;
      bump_inst_map_[bump.bump_inst] = &bump;
    }
  }
  chip_path_map_[chip.chip_inst_path] = &chip;
}

void UnfoldedModel::unfoldRegions(UnfoldedChip& uf_chip,
                                  dbChipInst* inst,
                                  const dbTransform& transform)
{
  for (auto* region_inst : inst->getRegions()) {
    auto side = region_inst->getChipRegion()->getSide();
    if (transform.isMirrorZ()) {
      side = mirrorSide(side);
    }

    UnfoldedRegion uf_region;
    uf_region.region_inst = region_inst;
    uf_region.effective_side = side;
    uf_region.cuboid = region_inst->getChipRegion()->getCuboid();

    transform.apply(uf_region.cuboid);
    unfoldBumps(uf_region, transform);
    uf_chip.regions.push_back(std::move(uf_region));
  }
}

void UnfoldedModel::unfoldBumps(UnfoldedRegion& uf_region,
                                const dbTransform& transform)
{
  const int z = uf_region.getSurfaceZ();
  for (auto* bump_inst : uf_region.region_inst->getChipBumpInsts()) {
    dbChipBump* bump = bump_inst->getChipBump();
    if (auto* inst = bump->getInst()) {
      Point global_xy = inst->getLocation();
      transform.apply(global_xy);
      uf_region.bumps.push_back(
          {.bump_inst = bump_inst,
           .parent_region = &uf_region,
           .global_position = Point3D(global_xy.x(), global_xy.y(), z)});
    }
  }
}

UnfoldedChip* UnfoldedModel::findUnfoldedChip(
    const std::vector<dbChipInst*>& path)
{
  auto it = chip_path_map_.find(path);
  return it != chip_path_map_.end() ? it->second : nullptr;
}

UnfoldedRegion* UnfoldedModel::findUnfoldedRegion(UnfoldedChip* chip,
                                                  dbChipRegionInst* inst)
{
  if (!chip || !inst) {
    return nullptr;
  }
  auto it = chip->region_map.find(inst);
  return it != chip->region_map.end() ? it->second : nullptr;
}

void UnfoldedModel::unfoldConnections(
    dbChip* chip,
    const std::vector<dbChipInst*>& parent_path)
{
  for (auto* conn : chip->getChipConns()) {
    UnfoldedRegion* top = findUnfoldedRegion(
        findUnfoldedChip(concatPath(parent_path, conn->getTopRegionPath())),
        conn->getTopRegion());
    UnfoldedRegion* bot = findUnfoldedRegion(
        findUnfoldedChip(concatPath(parent_path, conn->getBottomRegionPath())),
        conn->getBottomRegion());

    if (top || bot) {
      UnfoldedConnection uf_conn{
          .connection = conn, .top_region = top, .bottom_region = bot};
      if (top != nullptr && top->isInternalExt()) {
        top->isUsed = true;
      }
      if (bot != nullptr && bot->isInternalExt()) {
        bot->isUsed = true;
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

      auto iter = bump_inst_map_.find(b_inst);
      if (iter != bump_inst_map_.end()) {
        uf_net.connected_bumps.push_back(iter->second);
      }
    }
    unfolded_nets_.push_back(std::move(uf_net));
  }
}

}  // namespace odb

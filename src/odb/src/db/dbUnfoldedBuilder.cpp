// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#include "dbUnfoldedBuilder.h"

#include <cstdint>
#include <string>
#include <vector>

#include "dbChip.h"
#include "dbChipBumpInst.h"
#include "dbChipConn.h"
#include "dbChipInst.h"
#include "dbChipNet.h"
#include "dbChipRegion.h"
#include "dbChipRegionInst.h"
#include "dbDatabase.h"
#include "dbTable.h"
#include "dbUnfoldedChipBumpInst.h"
#include "dbUnfoldedChipConn.h"
#include "dbUnfoldedChipInst.h"
#include "dbUnfoldedChipNet.h"
#include "dbUnfoldedChipRegionInst.h"
#include "odb/db.h"
#include "odb/dbTransform.h"

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

dbUnfoldedChipRegionInst::EffectiveSide mirrorSide(
    dbUnfoldedChipRegionInst::EffectiveSide side)
{
  if (side == dbUnfoldedChipRegionInst::EffectiveSide::TOP) {
    return dbUnfoldedChipRegionInst::EffectiveSide::BOTTOM;
  }
  if (side == dbUnfoldedChipRegionInst::EffectiveSide::BOTTOM) {
    return dbUnfoldedChipRegionInst::EffectiveSide::TOP;
  }
  return side;
}

}  // namespace

dbUnfoldedBuilder::dbUnfoldedBuilder(_dbDatabase* db) : db_(db)
{
}

void dbUnfoldedBuilder::build()
{
  db_->unfolded_chip_inst_tbl_->clear();
  db_->unfolded_chip_region_inst_tbl_->clear();
  db_->unfolded_chip_bump_inst_tbl_->clear();
  db_->unfolded_chip_conn_tbl_->clear();
  db_->unfolded_chip_net_tbl_->clear();
  chip_by_path_.clear();
  region_map_.clear();
  bump_map_.clear();

  dbChip* chip = ((dbDatabase*) db_)->getChip();
  if (chip == nullptr) {
    return;
  }

  std::vector<dbChipInst*> path;
  for (dbChipInst* inst : chip->getChipInsts()) {
    buildUnfoldedChip(inst, path, dbTransform());
  }
  unfoldConnections(chip, {});
  unfoldNets(chip, {});
}

_dbUnfoldedChipInst* dbUnfoldedBuilder::buildUnfoldedChip(
    dbChipInst* inst,
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

  _dbUnfoldedChipInst* uf_chip = db_->unfolded_chip_inst_tbl_->create();
  uf_chip->name_ = getFullPathName(path);
  uf_chip->chip_inst_path_.reserve(path.size());
  for (auto* p : path) {
    uf_chip->chip_inst_path_.push_back(p->getImpl()->getOID());
  }
  uf_chip->transform_ = total;
  unfoldRegions(uf_chip, inst);

  chip_by_path_[uf_chip->name_] = uf_chip->getOID();

  path.pop_back();
  return uf_chip;
}

void dbUnfoldedBuilder::unfoldRegions(_dbUnfoldedChipInst* uf_chip,
                                      dbChipInst* inst)
{
  auto& chip_region_map = region_map_[uf_chip->getOID()];
  for (auto* region_inst : inst->getRegions()) {
    auto region = region_inst->getChipRegion();

    dbUnfoldedChipRegionInst::EffectiveSide side
        = dbUnfoldedChipRegionInst::EffectiveSide::INTERNAL;
    switch (region->getSide()) {
      case dbChipRegion::Side::FRONT:
        side = dbUnfoldedChipRegionInst::EffectiveSide::TOP;
        break;
      case dbChipRegion::Side::BACK:
        side = dbUnfoldedChipRegionInst::EffectiveSide::BOTTOM;
        break;
      case dbChipRegion::Side::INTERNAL:
        side = dbUnfoldedChipRegionInst::EffectiveSide::INTERNAL;
        break;
      case dbChipRegion::Side::INTERNAL_EXT:
        side = dbUnfoldedChipRegionInst::EffectiveSide::INTERNAL_EXT;
        break;
    }

    if (uf_chip->transform_.isMirrorZ()) {
      side = mirrorSide(side);
    }

    _dbUnfoldedChipRegionInst* uf_region
        = db_->unfolded_chip_region_inst_tbl_->create();
    uf_region->chip_region_inst_ = region_inst->getImpl()->getOID();
    uf_region->flags_.effective_side_ = static_cast<uint32_t>(side);
    uf_region->parent_chip_ = uf_chip->getOID();
    uf_region->chip_next_ = uf_chip->region_;
    uf_chip->region_ = uf_region->getOID();

    chip_region_map[region_inst] = uf_region->getOID();
    unfoldBumps(uf_region, region_inst);
  }
  // Inserts were head-first; reverse to restore source order.
  ((dbUnfoldedChipInst*) uf_chip)->getRegions().reverse();
}

void dbUnfoldedBuilder::unfoldBumps(_dbUnfoldedChipRegionInst* uf_region,
                                    dbChipRegionInst* region_inst)
{
  auto& chip_bump_map = bump_map_[uf_region->parent_chip_];
  for (auto* bump_inst : region_inst->getChipBumpInsts()) {
    dbChipBump* bump = bump_inst->getChipBump();
    if (bump->getInst() == nullptr) {
      continue;
    }
    _dbUnfoldedChipBumpInst* uf_bump
        = db_->unfolded_chip_bump_inst_tbl_->create();
    uf_bump->chip_bump_inst_ = bump_inst->getImpl()->getOID();
    uf_bump->parent_region_ = uf_region->getOID();
    uf_bump->region_next_ = uf_region->bump_;
    uf_region->bump_ = uf_bump->getOID();

    chip_bump_map[bump_inst] = uf_bump->getOID();
  }
  // Inserts were head-first; reverse to restore source order.
  ((dbUnfoldedChipRegionInst*) uf_region)->getBumps().reverse();
}

_dbUnfoldedChipInst* dbUnfoldedBuilder::findUnfoldedChip(
    const std::vector<dbChipInst*>& path)
{
  auto it = chip_by_path_.find(getFullPathName(path));
  if (it == chip_by_path_.end()) {
    return nullptr;
  }
  return db_->unfolded_chip_inst_tbl_->getPtr(it->second);
}

void dbUnfoldedBuilder::unfoldConnections(
    dbChip* chip,
    const std::vector<dbChipInst*>& parent_path)
{
  for (auto* conn : chip->getChipConns()) {
    dbId<_dbUnfoldedChipRegionInst> top_region = 0;
    if (_dbUnfoldedChipInst* top_chip
        = findUnfoldedChip(concatPath(parent_path, conn->getTopRegionPath()))) {
      auto& map = region_map_[top_chip->getOID()];
      auto it = map.find(conn->getTopRegion());
      if (it != map.end()) {
        top_region = it->second;
      }
    }
    dbId<_dbUnfoldedChipRegionInst> bot_region = 0;
    if (_dbUnfoldedChipInst* bot_chip = findUnfoldedChip(
            concatPath(parent_path, conn->getBottomRegionPath()))) {
      auto& map = region_map_[bot_chip->getOID()];
      auto it = map.find(conn->getBottomRegion());
      if (it != map.end()) {
        bot_region = it->second;
      }
    }
    if (top_region.isValid() || bot_region.isValid()) {
      _dbUnfoldedChipConn* uf_conn = db_->unfolded_chip_conn_tbl_->create();
      uf_conn->chip_conn_ = conn->getImpl()->getOID();
      uf_conn->top_region_ = top_region;
      uf_conn->bottom_region_ = bot_region;
    }
  }
}

void dbUnfoldedBuilder::unfoldNets(dbChip* chip,
                                   const std::vector<dbChipInst*>& parent_path)
{
  for (auto* net : chip->getChipNets()) {
    _dbUnfoldedChipNet* uf_net = db_->unfolded_chip_net_tbl_->create();
    uf_net->chip_net_ = net->getImpl()->getOID();
    for (uint32_t i = 0; i < net->getNumBumpInsts(); i++) {
      std::vector<dbChipInst*> rel_path;
      dbChipBumpInst* b_inst = net->getBumpInst(i, rel_path);
      _dbUnfoldedChipInst* uf_chip
          = findUnfoldedChip(concatPath(parent_path, rel_path));
      if (uf_chip == nullptr) {
        continue;
      }
      auto& map = bump_map_[uf_chip->getOID()];
      auto it = map.find(b_inst);
      if (it != map.end()) {
        uf_net->connected_bumps_.push_back(it->second);
      }
    }
  }
}

}  // namespace odb

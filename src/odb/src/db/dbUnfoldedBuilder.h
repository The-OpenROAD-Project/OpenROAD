// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, The OpenROAD Authors

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "odb/dbId.h"
#include "odb/dbTransform.h"

namespace odb {

class _dbDatabase;
class _dbUnfoldedBump;
class _dbUnfoldedChip;
class _dbUnfoldedRegion;
class dbChip;
class dbChipBumpInst;
class dbChipInst;
class dbChipRegionInst;

class dbUnfoldedBuilder
{
 public:
  explicit dbUnfoldedBuilder(_dbDatabase* db);
  void build();

 private:
  _dbUnfoldedChip* buildUnfoldedChip(dbChipInst* inst,
                                     std::vector<dbChipInst*>& path,
                                     const dbTransform& parent_xform);
  void unfoldRegions(_dbUnfoldedChip* uf_chip, dbChipInst* inst);
  void unfoldBumps(_dbUnfoldedRegion* uf_region, dbChipRegionInst* region_inst);
  void unfoldConnections(dbChip* chip,
                         const std::vector<dbChipInst*>& parent_path);
  void unfoldNets(dbChip* chip, const std::vector<dbChipInst*>& parent_path);

  _dbUnfoldedChip* findUnfoldedChip(const std::vector<dbChipInst*>& path);

  _dbDatabase* db_;
  std::unordered_map<std::string, dbId<_dbUnfoldedChip>> chip_by_path_;
  std::unordered_map<
      uint32_t,
      std::unordered_map<dbChipRegionInst*, dbId<_dbUnfoldedRegion>>>
      region_map_;
  std::unordered_map<uint32_t,
                     std::unordered_map<dbChipBumpInst*, dbId<_dbUnfoldedBump>>>
      bump_map_;
};

}  // namespace odb

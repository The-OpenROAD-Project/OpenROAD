// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "dbTransform.h"
#include "geom.h"

namespace odb {
class dbChip;
class dbChipInst;
class dbChipRegion;
class dbChipRegionInst;
class dbChipBumpInst;
class dbChipConn;
class dbChipNet;

enum class UnfoldedRegionSide
{
  TOP,
  BOTTOM,
  INTERNAL,
  INTERNAL_EXT
};

struct UnfoldedChip;
struct UnfoldedRegion;

struct UnfoldedBump
{
  dbChipBumpInst* bump_inst = nullptr;
  UnfoldedRegion* parent_region = nullptr;
  Point3D global_position;
};

struct UnfoldedRegion
{
  dbChipRegionInst* region_inst = nullptr;
  UnfoldedRegionSide effective_side = UnfoldedRegionSide::TOP;
  Cuboid cuboid;
  UnfoldedChip* parent_chip = nullptr;
  std::deque<UnfoldedBump> bumps;
  bool isUsed = false;

  int getSurfaceZ() const;
  bool isTop() const { return effective_side == UnfoldedRegionSide::TOP; }
  bool isBottom() const { return effective_side == UnfoldedRegionSide::BOTTOM; }
  bool isInternal() const
  {
    return effective_side == UnfoldedRegionSide::INTERNAL;
  }
  bool isInternalExt() const
  {
    return effective_side == UnfoldedRegionSide::INTERNAL_EXT;
  }
};

struct UnfoldedConnection
{
  dbChipConn* connection = nullptr;
  UnfoldedRegion* top_region = nullptr;
  UnfoldedRegion* bottom_region = nullptr;
};

struct UnfoldedNet
{
  dbChipNet* chip_net = nullptr;
  std::vector<UnfoldedBump*> connected_bumps;
};

struct UnfoldedChip
{
  std::string name;

  std::vector<dbChipInst*> chip_inst_path;
  Cuboid cuboid;
  dbTransform transform;

  std::deque<UnfoldedRegion> regions;

  std::unordered_map<dbChipRegionInst*, UnfoldedRegion*> region_map;
};

class UnfoldedModel
{
 public:
  UnfoldedModel(utl::Logger* logger, dbChip* chip);

  const std::deque<UnfoldedChip>& getChips() const { return unfolded_chips_; }
  const std::vector<UnfoldedConnection>& getConnections() const
  {
    return unfolded_connections_;
  }
  const std::vector<UnfoldedNet>& getNets() const { return unfolded_nets_; }

 private:
  UnfoldedChip* buildUnfoldedChip(dbChipInst* chip_inst,
                                  std::vector<dbChipInst*>& path,
                                  const dbTransform& parent_xform);
  void registerUnfoldedChip(UnfoldedChip& uf_chip);
  void unfoldRegions(UnfoldedChip& uf_chip, dbChipInst* inst);
  void unfoldBumps(UnfoldedRegion& uf_region, const dbTransform& transform);
  void unfoldConnections(dbChip* chip,
                         const std::vector<dbChipInst*>& parent_path);
  void unfoldNets(dbChip* chip, const std::vector<dbChipInst*>& parent_path);

  UnfoldedChip* findUnfoldedChip(const std::vector<dbChipInst*>& path);
  UnfoldedRegion* findUnfoldedRegion(UnfoldedChip* chip,
                                     dbChipRegionInst* region_inst);

  utl::Logger* logger_;
  std::deque<UnfoldedChip> unfolded_chips_;
  std::vector<UnfoldedConnection> unfolded_connections_;
  std::vector<UnfoldedNet> unfolded_nets_;
  std::map<std::vector<dbChipInst*>, UnfoldedChip*> chip_path_map_;
  std::map<std::pair<dbChipBumpInst*, std::vector<dbChipInst*>>, UnfoldedBump*>
      bump_inst_map_;
};

}  // namespace odb

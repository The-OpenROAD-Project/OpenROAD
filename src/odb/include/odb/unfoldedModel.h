// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "odb/dbBlockCallBackObj.h"
#include "odb/dbChipCallBackObj.h"
#include "odb/dbTransform.h"
#include "odb/geom.h"
#include "utl/Logger.h"

namespace odb {
class dbBlock;
class dbChip;
class dbChipInst;
class dbChipRegion;
class dbChipRegionInst;
class dbChipBumpInst;
class dbChipConn;
class dbChipNet;
class UnfoldedModel;

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
  Point3D getGlobalPosition() const;
};

struct UnfoldedRegion
{
  dbChipRegionInst* region_inst = nullptr;
  UnfoldedRegionSide effective_side = UnfoldedRegionSide::TOP;
  UnfoldedChip* parent_chip = nullptr;
  std::deque<UnfoldedBump> bumps;
  bool isUsed = false;
  Cuboid getCuboid() const;
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
  std::vector<dbChipInst*> chip_inst_path;
  dbTransform transform;

  std::deque<UnfoldedRegion> regions;

  std::unordered_map<dbChipRegionInst*, UnfoldedRegion*> region_map;
  std::unordered_map<dbChipBumpInst*, UnfoldedBump*> bump_inst_map;

  UnfoldedRegion* findUnfoldedRegion(dbChipRegionInst* region_inst);
  UnfoldedBump* findUnfoldedBump(dbChipBumpInst* bump_inst);
  Cuboid getCuboid() const;
  std::string getFullName() const;
};

class UnfoldedChipObserver : public dbChipCallBackObj
{
 public:
  explicit UnfoldedChipObserver(UnfoldedModel* model) : model_(model) {}

 private:
  UnfoldedModel* model_;
};

class UnfoldedBlockObserver : public dbBlockCallBackObj
{
 public:
  explicit UnfoldedBlockObserver(UnfoldedModel* model) : model_(model) {}

 private:
  UnfoldedModel* model_;
};

class UnfoldedModel
{
 public:
  UnfoldedModel(utl::Logger* logger, dbChip* chip);

  const std::vector<std::unique_ptr<UnfoldedChip>>& getChips() const
  {
    return unfolded_chips_;
  }
  const std::vector<UnfoldedConnection>& getConnections() const
  {
    return unfolded_connections_;
  }
  const std::vector<UnfoldedNet>& getNets() const { return unfolded_nets_; }
  UnfoldedChip* findUnfoldedChip(const std::string& path);

 private:
  UnfoldedChip* buildUnfoldedChip(dbChipInst* chip_inst,
                                  std::vector<dbChipInst*>& path,
                                  const dbTransform& parent_xform);
  void registerUnfoldedChip(UnfoldedChip* uf_chip);
  void unfoldRegions(UnfoldedChip* uf_chip, dbChipInst* inst);
  void unfoldBumps(UnfoldedRegion& uf_region, const dbTransform& transform);
  void unfoldConnections(dbChip* chip,
                         const std::vector<dbChipInst*>& parent_path);
  void unfoldNets(dbChip* chip, const std::vector<dbChipInst*>& parent_path);

  UnfoldedChip* findUnfoldedChip(const std::vector<dbChipInst*>& path);

  // Recursively register chip/block observers across the hierarchy rooted
  // at `chip` so that callback events can keep the unfolded model in sync.
  void attachObservers(dbChip* chip);

  utl::Logger* logger_;
  std::vector<std::unique_ptr<UnfoldedChip>> unfolded_chips_;
  std::vector<UnfoldedConnection> unfolded_connections_;
  std::vector<UnfoldedNet> unfolded_nets_;
  std::map<std::string, UnfoldedChip*> chip_map_;

  // Use deque so observer addresses are stable; ~CallBackObj removes
  // ownership automatically when the model is destroyed.
  std::deque<UnfoldedChipObserver> chip_observers_;
  std::deque<UnfoldedBlockObserver> block_observers_;
  std::unordered_set<dbChip*> observed_chips_;
  std::unordered_set<dbBlock*> observed_blocks_;
};

}  // namespace odb

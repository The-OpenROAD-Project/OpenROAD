// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

#include "odb/3dblox.h"
#include "odb/db.h"
#include "odb/geom.h"

namespace odb {
class dbChip;
class dbChipInst;
class dbChipRegion;
class dbChipRegionInst;
class dbChipBumpInst;
class dbChipConn;
class dbChipNet;

struct UnfoldedChip;

struct UnfoldedRegion
{
  dbChipRegionInst* region_inst;
  dbChipRegion::Side effective_side;
  Cuboid cuboid;
  UnfoldedChip* parent_chip = nullptr;
};

struct UnfoldedBump
{
  dbChipBumpInst* bump_inst;
  UnfoldedRegion* parent_region;
  Point3D global_position;
  std::string logical_net_name;
  std::string port_name;
};

struct UnfoldedRegionFull : public UnfoldedRegion
{
  std::deque<UnfoldedBump> bumps;

  int getSurfaceZ() const;
  bool isFacingUp() const;
  bool isFacingDown() const;
};

struct UnfoldedConnection
{
  dbChipConn* connection;
  UnfoldedRegionFull* top_region;
  UnfoldedRegionFull* bottom_region;
  Cuboid connection_cuboid;

  bool isValid() const;
};

struct UnfoldedNet
{
  dbChipNet* chip_net;
  std::vector<UnfoldedBump*> connected_bumps;

  std::vector<UnfoldedBump*> getDisconnectedBumps(
      int bump_pitch_tolerance) const;
};

struct UnfoldedChip
{
  std::string getName() const;
  std::string getPathKey() const;

  std::vector<dbChipInst*> chip_inst_path;
  Cuboid cuboid;

  bool z_flipped = false;
  std::deque<UnfoldedRegionFull> regions;
  std::vector<UnfoldedConnection*> connected_conns;
};

class UnfoldedModel
{
 public:
  UnfoldedModel(utl::Logger* logger);
  void build(dbChip* chip);

  const std::deque<UnfoldedChip>& getChips() const { return unfolded_chips_; }
  const std::deque<UnfoldedConnection>& getConnections() const
  {
    return unfolded_connections_;
  }
  const std::deque<UnfoldedNet>& getNets() const { return unfolded_nets_; }

 private:
  void unfoldChip(dbChipInst* chip_inst, UnfoldedChip& unfolded_chip);
  void unfoldBumps(UnfoldedRegionFull& uf_region,
                   const std::vector<dbChipInst*>& path);
  void unfoldConnections(dbChip* chip);
  void unfoldConnectionsRecursive(dbChip* chip,
                                  const std::vector<dbChipInst*>& parent_path);
  void unfoldNets(dbChip* chip);

  UnfoldedChip* findUnfoldedChip(const std::vector<dbChipInst*>& path);
  UnfoldedRegionFull* findUnfoldedRegion(UnfoldedChip* chip,
                                         dbChipRegionInst* region_inst);

  utl::Logger* logger_;
  std::deque<UnfoldedChip> unfolded_chips_;
  std::deque<UnfoldedConnection> unfolded_connections_;
  std::deque<UnfoldedNet> unfolded_nets_;
  std::map<std::string, UnfoldedChip*> chip_path_map_;
};

}  // namespace odb

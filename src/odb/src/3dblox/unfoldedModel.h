// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2026, The OpenROAD Authors

#pragma once

#include <deque>
#include <map>
#include <string>
#include <unordered_map>
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
  dbChipRegionInst* region_inst;  // non-owning, managed by dbChip
  dbChipRegion::Side effective_side;
  Cuboid cuboid;
  UnfoldedChip* parent_chip
      = nullptr;  // non-owning, points to UnfoldedModel::unfolded_chips_
};

struct UnfoldedBump
{
  dbChipBumpInst* bump_inst;      // non-owning, managed by dbChip
  UnfoldedRegion* parent_region;  // non-owning, points to UnfoldedChip::regions
  Point3D global_position;
  std::string logical_net_name;
  std::string port_name;
};

struct UnfoldedRegionFull : public UnfoldedRegion
{
  std::deque<UnfoldedBump> bumps;
  bool isUsed = false;

  int getSurfaceZ() const;
  bool isFacingUp() const;
  bool isFacingDown() const;
  bool isInternal() const;
  bool isInternalExt() const;
};

struct UnfoldedConnection
{
  dbChipConn* connection;  // non-owning, managed by dbChip
  UnfoldedRegionFull*
      top_region;  // non-owning, may be null for virtual connections
  UnfoldedRegionFull*
      bottom_region;  // non-owning, may be null for virtual connections
  Cuboid connection_cuboid;
  bool is_bterm_connection = false;
  dbBTerm* bterm = nullptr;  // non-owning, managed by dbBlock

  bool isValid() const;
};

struct UnfoldedNet
{
  dbChipNet* chip_net;  // non-owning, managed by dbChip
  std::vector<UnfoldedBump*>
      connected_bumps;  // non-owning, points to UnfoldedRegionFull::bumps

  std::vector<UnfoldedBump*> getDisconnectedBumps(
      utl::Logger* logger,
      const std::deque<UnfoldedConnection>& connections,
      int bump_pitch_tolerance) const;
};

struct UnfoldedChip
{
  std::string getName() const;
  std::string getPathKey() const;
  bool isParentOf(const UnfoldedChip* other) const;

  std::vector<dbChipInst*> chip_inst_path;  // non-owning, managed by dbChip
  Cuboid cuboid;

  bool z_flipped = false;
  std::deque<UnfoldedRegionFull> regions;  // owning container
  std::vector<UnfoldedConnection*>
      connected_conns;  // non-owning, points to unfolded_connections_

  std::unordered_map<dbChipRegionInst*, UnfoldedRegionFull*> region_map;
};

class UnfoldedModel
{
 public:
  UnfoldedModel(utl::Logger* logger, dbChip* chip);

  // Accessors return const references to internal containers.
  // The containers use std::deque to ensure pointer stability when elements
  // are added, allowing other structures to hold raw pointers to elements.
  const std::deque<UnfoldedChip>& getChips() const { return unfolded_chips_; }
  const std::deque<UnfoldedConnection>& getConnections() const
  {
    return unfolded_connections_;
  }
  const std::deque<UnfoldedNet>& getNets() const { return unfolded_nets_; }

 private:
  UnfoldedChip* buildUnfoldedChip(dbChipInst* chip_inst,
                                  std::vector<dbChipInst*>& path,
                                  Cuboid& local_cuboid);
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

  // Using std::deque for pointer stability: elements maintain their addresses
  // even when the container grows, which is critical since UnfoldedRegion,
  // UnfoldedBump, etc. hold raw pointers to their parent structures.
  std::deque<UnfoldedChip> unfolded_chips_;
  std::deque<UnfoldedConnection> unfolded_connections_;
  std::deque<UnfoldedNet> unfolded_nets_;
  std::map<std::string, UnfoldedChip*> chip_path_map_;
  std::unordered_map<dbChipBumpInst*, UnfoldedBump*> bump_inst_map_;
};

}  // namespace odb

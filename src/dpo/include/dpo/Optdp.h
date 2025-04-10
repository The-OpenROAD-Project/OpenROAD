// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2021-2025, The OpenROAD Authors

#pragma once

#include <functional>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>  // pair
#include <vector>

#include "odb/db.h"

namespace utl {
class Logger;
}

namespace dpl {
class Opendp;
class Grid;
}  // namespace dpl

namespace dpo {

class RoutingParams;
class Architecture;
class Network;
class Node;
class Edge;
class Pin;
class Master;

using dpl::Grid;
using dpl::Opendp;
using odb::dbDatabase;
using odb::dbOrientType;
using utl::Logger;

class Optdp
{
 public:
  Optdp() = default;

  Optdp(const Optdp&) = delete;
  Optdp& operator=(const Optdp&) = delete;
  Optdp(const Optdp&&) = delete;
  Optdp& operator=(const Optdp&&) = delete;

  void init(odb::dbDatabase* db, utl::Logger* logger, dpl::Opendp* opendp);

  void improvePlacement(int seed,
                        int max_displacement_x,
                        int max_displacement_y);

 private:
  void import();
  void updateDbInstLocations();

  void initSpacingTable();
  void initPadding();
  void createLayerMap();
  void createNdrMap();
  void setupMasterPowers();
  void createNetwork();
  void createArchitecture();
  void createRouteInformation();
  void createGrid();
  void setUpNdrRules();
  void setUpPlacementRegions();
  Master* getMaster(odb::dbMaster* db_master);

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;

  // My stuff.
  Architecture* arch_ = nullptr;  // Information about rows, etc.
  Network* network_ = nullptr;    // The netlist, cells, etc.
  RoutingParams* routeinfo_
      = nullptr;  // Route info we might consider (future).
  Grid* grid_ = nullptr;

  // Some maps.
  std::unordered_map<odb::dbInst*, Node*> instMap_;
  std::unordered_map<odb::dbNet*, Edge*> netMap_;
  std::unordered_map<odb::dbBTerm*, Node*> termMap_;
  std::unordered_map<odb::dbMaster*, Master*> masterMap_;

  // For monitoring power alignment.
  std::unordered_set<odb::dbTechLayer*> pwrLayers_;
  std::unordered_set<odb::dbTechLayer*> gndLayers_;
  std::unordered_map<odb::dbMaster*, std::pair<int, int>>
      masterPwrs_;  // top,bot
};

}  // namespace dpo

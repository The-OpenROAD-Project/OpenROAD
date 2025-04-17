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
class Master;
class Edge;
class Node;
class PlacementDRC;

}  // namespace dpl

namespace dpo {

class Architecture;
class Network;

using dpl::Edge;
using dpl::Grid;
using dpl::Node;
using dpl::Opendp;
using dpl::PlacementDRC;
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

  void initPlacementDRC();
  void initPadding();
  void createLayerMap();
  void createNdrMap();
  void setupMasterPowers();
  void createNetwork();
  void createArchitecture();
  void createRouteInformation();
  void createGrid();
  void setUpNdrRules();
  void setUpPlacementGroups();
  dpl::Master* getMaster(odb::dbMaster* db_master);

  odb::dbDatabase* db_ = nullptr;
  utl::Logger* logger_ = nullptr;
  dpl::Opendp* opendp_ = nullptr;

  // My stuff.
  Architecture* arch_ = nullptr;  // Information about rows, etc.
  Network* network_ = nullptr;    // The netlist, cells, etc.
  Grid* grid_ = nullptr;

  // placement DRC enging.
  PlacementDRC* drc_engine_ = nullptr;

  // Some maps.
  std::unordered_map<odb::dbInst*, Node*> instMap_;
  std::unordered_map<odb::dbNet*, Edge*> netMap_;
  std::unordered_map<odb::dbBTerm*, Node*> termMap_;
  std::unordered_map<odb::dbMaster*, dpl::Master*> masterMap_;

  // For monitoring power alignment.
  std::unordered_set<odb::dbTechLayer*> pwrLayers_;
  std::unordered_set<odb::dbTechLayer*> gndLayers_;
  std::unordered_map<odb::dbMaster*, std::pair<int, int>>
      masterPwrs_;  // top,bot
};

}  // namespace dpo

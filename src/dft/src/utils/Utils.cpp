// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "Utils.hh"

#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/SdcClass.hh"
#include "utl/Logger.h"

namespace dft::utils {

namespace {

void PopulatePortNameToNet(
    odb::dbInst* instance,
    std::vector<std::tuple<std::string, odb::dbNet*>>& port_name_to_net)
{
  for (odb::dbITerm* iterm : instance->getITerms()) {
    port_name_to_net.emplace_back(iterm->getMTerm()->getName(),
                                  iterm->getNet());
  }
}

void ConnectPinsToNets(
    odb::dbInst* instance,
    const std::vector<std::tuple<std::string, odb::dbNet*>>& port_name_to_net,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
  for (const auto& [port_name_old, net] : port_name_to_net) {
    if (net == nullptr) {
      continue;
    }
    std::string port_name_new = port_mapping.find(port_name_old)->second;
    instance->findITerm(port_name_new.c_str())->connect(net);
  }
}

}  // namespace

bool IsSequentialCell(sta::dbNetwork* db_network, odb::dbInst* instance)
{
  odb::dbMaster* master = instance->getMaster();
  sta::Cell* master_cell = db_network->dbToSta(master);
  sta::LibertyCell* liberty_cell = db_network->libertyCell(master_cell);
  return liberty_cell->hasSequentials();
}

odb::dbInst* ReplaceCell(
    odb::dbBlock* top_block,
    odb::dbInst* old_instance,
    odb::dbMaster* new_master,
    const std::unordered_map<std::string, std::string>& port_mapping)
{
  std::vector<std::tuple<std::string, odb::dbNet*>> port_name_to_net;
  PopulatePortNameToNet(old_instance, port_name_to_net);

  const std::string cell_name = old_instance->getName();
  const odb::dbPlacementStatus placement_status
      = old_instance->getPlacementStatus();
  const odb::dbSourceType source_type = old_instance->getSourceType();
  odb::dbRegion* region = old_instance->getRegion();
  odb::dbGroup* group = old_instance->getGroup();
  odb::dbModule* module = old_instance->getModule();

  odb::dbInst* new_instance = odb::dbInst::create(top_block,
                                                  new_master,
                                                  /*name=*/"tmp_scan_flop",
                                                  /*physical_only=*/false,
                                                  module);

  new_instance->setTransform(old_instance->getTransform());
  new_instance->setPlacementStatus(placement_status);
  new_instance->setSourceType(source_type);
  if (region) {
    region->addInst(new_instance);
  }
  if (group) {
    group->addInst(new_instance);
  }

  // Delete the old cell
  odb::dbInst::destroy(old_instance);

  // Connect the new cell to the old instance's nets
  ConnectPinsToNets(new_instance, port_name_to_net, port_mapping);

  // Rename as the old cell
  new_instance->rename(cell_name.c_str());

  return new_instance;
}

std::vector<odb::dbITerm*> GetClockPin(odb::dbInst* inst)
{
  std::vector<odb::dbITerm*> clocks;
  for (odb::dbITerm* iterm : inst->getITerms()) {
    if (iterm->getSigType() == odb::dbSigType::CLOCK) {
      clocks.push_back(iterm);
    }
  }
  return clocks;
}

std::optional<sta::Clock*> GetClock(sta::dbSta* sta, odb::dbITerm* iterm)
{
  const sta::dbNetwork* db_network = sta->getDbNetwork();
  const sta::ClockSet clock_set
      = sta->clocks(db_network->dbToSta(iterm), sta->cmdMode());

  if (!clock_set.empty()) {
    // Returns the first clock for the given iterm, TODO can we have more than
    // one clock driver?
    return *clock_set.begin();
  }

  return std::nullopt;
}

bool IsScanCell(const sta::LibertyCell* liberty_cell)
{
  const sta::TestCell* test_cell = liberty_cell->testCell();
  if (test_cell) {
    return getLibertyScanIn(test_cell) != nullptr
           && getLibertyScanEnable(test_cell) != nullptr;
  }
  return false;
}

odb::dbBTerm* CreateNewPort(odb::dbBlock* block,
                            const std::string& port_name,
                            utl::Logger* logger,
                            odb::dbNet* net)
{
  if (!net) {
    net = odb::dbNet::create(block, port_name.c_str());
    if (!net) {
      logger->error(utl::DFT,
                    31,
                    "Error while attempting to create new port {}: an "
                    "unrelated net with the same name already exists",
                    port_name);
    }
    net->setSigType(odb::dbSigType::SCAN);
  }
  auto port = odb::dbBTerm::create(net, port_name.c_str());
  if (port == nullptr) {
    logger->error(utl::DFT,
                  18,
                  "Failed to create port: a port named '{}' already exists",
                  port_name);
  }
  port->setSigType(odb::dbSigType::SCAN);

  return port;
}

}  // namespace dft::utils

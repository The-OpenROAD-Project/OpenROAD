// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2023-2025, The OpenROAD Authors

#include "Utils.hh"

#include <algorithm>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "db_sta/dbNetwork.hh"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "odb/dbTransform.h"
#include "odb/dbTypes.h"
#include "sta/ClkNetwork.hh"
#include "sta/DisabledPorts.hh"
#include "sta/Graph.hh"
#include "sta/Liberty.hh"
#include "sta/Mode.hh"
#include "sta/NetworkClass.hh"
#include "sta/Sdc.hh"
#include "sta/SdcClass.hh"
#include "sta/TimingRole.hh"
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

using PortMapping = std::unordered_map<std::string, std::string>;

bool IsRenamed(const PortMapping& port_mapping, const sta::LibertyPort* port)
{
  const auto mapped = port_mapping.find(port->name());
  return mapped != port_mapping.end() && mapped->second != port->name();
}

// The port of new_cell that port maps to, or null if there is none.
sta::LibertyPort* MappedPort(const PortMapping& port_mapping,
                             const sta::LibertyCell* new_cell,
                             const sta::LibertyPort* port)
{
  const auto mapped = port_mapping.find(port->name());
  if (mapped == port_mapping.end()) {
    return nullptr;
  }
  return new_cell->findLibertyPort(mapped->second);
}

// Move the instance set_disable_timing -from/-to with a renamed port. A
// disable without a mapped port is dropped.
void MoveRenamedInstPortDisables(sta::dbSta* sta,
                                 sta::Sdc* sdc,
                                 sta::Instance* old_inst,
                                 sta::Instance* new_inst,
                                 const PortMapping& port_mapping)
{
  const auto disabled_itr = sdc->disabledInstancePorts()->find(old_inst);
  if (disabled_itr == sdc->disabledInstancePorts()->end()) {
    return;
  }
  const sta::DisabledInstancePorts* disabled = disabled_itr->second;
  // From/to pairs, with null for no port.
  std::vector<std::pair<const sta::LibertyPort*, const sta::LibertyPort*>>
      port_disables;
  if (disabled->from()) {
    for (const sta::LibertyPort* port : *disabled->from()) {
      port_disables.emplace_back(port, nullptr);
    }
  }
  if (disabled->to()) {
    for (const sta::LibertyPort* port : *disabled->to()) {
      port_disables.emplace_back(nullptr, port);
    }
  }
  if (disabled->fromTo()) {
    port_disables.insert(port_disables.end(),
                         disabled->fromTo()->begin(),
                         disabled->fromTo()->end());
  }
  const sta::LibertyCell* old_cell = sta->getDbNetwork()->libertyCell(old_inst);
  const sta::LibertyCell* new_cell = sta->getDbNetwork()->libertyCell(new_inst);
  for (const auto& [from, to] : port_disables) {
    if ((from && IsRenamed(port_mapping, from))
        || (to && IsRenamed(port_mapping, to))) {
      sta->removeDisable(
          old_inst,
          from ? old_cell->findLibertyPort(from->name()) : nullptr,
          to ? old_cell->findLibertyPort(to->name()) : nullptr,
          sdc);
      sta::LibertyPort* new_from
          = from ? MappedPort(port_mapping, new_cell, from) : nullptr;
      sta::LibertyPort* new_to
          = to ? MappedPort(port_mapping, new_cell, to) : nullptr;
      if ((from == nullptr || new_from) && (to == nullptr || new_to)) {
        sta->disable(new_inst, new_from, new_to, sdc);
      }
    }
  }
}

// Move the disabled timing arcs of old_inst with a renamed end. An arc
// without a matching arc on new_inst is dropped.
void MoveRenamedArcDisables(sta::dbSta* sta,
                            sta::Sdc* sdc,
                            const sta::Instance* old_inst,
                            const sta::Instance* new_inst,
                            const PortMapping& port_mapping)
{
  const sta::dbNetwork* network = sta->getDbNetwork();
  const sta::LibertyCell* new_cell = network->libertyCell(new_inst);
  sta::Graph* graph = sta->graph();
  if (graph == nullptr) {
    return;
  }
  std::vector<sta::Edge*> arc_edges;
  for (sta::Edge* edge : *sdc->disabledEdges()) {
    const sta::Pin* from_pin = edge->from(graph)->pin();
    const sta::Pin* to_pin = edge->to(graph)->pin();
    if (!edge->role()->isWire() && network->instance(from_pin) == old_inst
        && (IsRenamed(port_mapping, network->libertyPort(from_pin))
            || IsRenamed(port_mapping, network->libertyPort(to_pin)))) {
      arc_edges.push_back(edge);
    }
  }
  for (sta::Edge* edge : arc_edges) {
    sta->removeDisable(edge, sdc);
    const sta::LibertyPort* from_port = MappedPort(
        port_mapping, new_cell, network->libertyPort(edge->from(graph)->pin()));
    const sta::LibertyPort* to_port = MappedPort(
        port_mapping, new_cell, network->libertyPort(edge->to(graph)->pin()));
    const sta::Pin* from_pin
        = from_port ? network->findPin(new_inst, from_port) : nullptr;
    const sta::Pin* to_pin
        = to_port ? network->findPin(new_inst, to_port) : nullptr;
    if (from_pin == nullptr || to_pin == nullptr) {
      continue;
    }
    sta::Vertex* from_vertex = graph->pinDrvrVertex(from_pin);
    sta::Vertex* to_vertex = graph->pinLoadVertex(to_pin);
    if (from_vertex == nullptr || to_vertex == nullptr) {
      continue;
    }
    sta::VertexOutEdgeIterator edge_iter(from_vertex, graph);
    while (edge_iter.hasNext()) {
      sta::Edge* new_edge = edge_iter.next();
      if (new_edge->to(graph) == to_vertex
          && new_edge->role() == edge->role()) {
        sta->disable(new_edge, sdc);
      }
    }
  }
}

// Sta::moveConstraints(Instance) matches ports by name, so it would drop or
// misplace the set_disable_timing on the ports that port_mapping renames.
// Move those to the mapped ports of new_instance first.
void MoveRenamedPortDisables(sta::dbSta* sta,
                             odb::dbInst* old_instance,
                             odb::dbInst* new_instance,
                             const PortMapping& port_mapping)
{
  if (std::ranges::none_of(port_mapping, [](const auto& mapping) {
        return mapping.first != mapping.second;
      })) {
    return;
  }
  sta::Instance* old_inst = sta->getDbNetwork()->dbToSta(old_instance);
  sta::Instance* new_inst = sta->getDbNetwork()->dbToSta(new_instance);
  for (sta::Mode* mode : sta->modes()) {
    MoveRenamedInstPortDisables(
        sta, mode->sdc(), old_inst, new_inst, port_mapping);
    MoveRenamedArcDisables(sta, mode->sdc(), old_inst, new_inst, port_mapping);
    for (const auto& [corner, corner_sdc] : mode->cornerSdcs()) {
      MoveRenamedInstPortDisables(
          sta, corner_sdc, old_inst, new_inst, port_mapping);
      MoveRenamedArcDisables(sta, corner_sdc, old_inst, new_inst, port_mapping);
    }
  }
}

}  // namespace

bool IsSequentialCell(sta::dbNetwork* db_network, odb::dbInst* instance)
{
  odb::dbMaster* master = instance->getMaster();
  sta::Cell* master_cell = db_network->dbToSta(master);
  sta::LibertyCell* liberty_cell = db_network->libertyCell(master_cell);
  return liberty_cell->isSequential();
}

odb::dbInst* ReplaceCell(
    sta::dbSta* sta,
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

  for (odb::dbProperty* prop : odb::dbProperty::getProperties(old_instance)) {
    const std::string name = prop->getName();
    switch (prop->getType()) {
      case odb::dbProperty::STRING_PROP:
        odb::dbStringProperty::create(
            new_instance,
            name.c_str(),
            static_cast<odb::dbStringProperty*>(prop)->getValue().c_str());
        break;
      case odb::dbProperty::BOOL_PROP:
        odb::dbBoolProperty::create(
            new_instance,
            name.c_str(),
            static_cast<odb::dbBoolProperty*>(prop)->getValue());
        break;
      case odb::dbProperty::INT_PROP:
        odb::dbIntProperty::create(
            new_instance,
            name.c_str(),
            static_cast<odb::dbIntProperty*>(prop)->getValue());
        break;
      case odb::dbProperty::DOUBLE_PROP:
        odb::dbDoubleProperty::create(
            new_instance,
            name.c_str(),
            static_cast<odb::dbDoubleProperty*>(prop)->getValue());
        break;
    }
  }

  // Connect the new cell to the old instance's nets, so the constraints on
  // the wire edges of the old pins can move to the new pins
  ConnectPinsToNets(new_instance, port_name_to_net, port_mapping);

  // Keep the activity annotations and SDC constraints of the old pins and
  // instance on the new ones
  for (odb::dbITerm* old_iterm : old_instance->getITerms()) {
    auto mapped = port_mapping.find(old_iterm->getMTerm()->getName());
    if (mapped != port_mapping.end()) {
      odb::dbITerm* new_iterm = new_instance->findITerm(mapped->second.c_str());
      if (new_iterm) {
        sta->copyUserActivity(old_iterm, new_iterm);
        sta->moveConstraints(old_iterm, new_iterm);
      }
    }
  }
  MoveRenamedPortDisables(sta, old_instance, new_instance, port_mapping);
  sta->moveConstraints(old_instance, new_instance);

  // Delete the old cell
  odb::dbInst::destroy(old_instance);

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
  const sta::Pin* pin = db_network->dbToSta(iterm);
  if (pin == nullptr) {
    return std::nullopt;
  }
  const sta::Mode* mode = sta->cmdMode();
  sta->ensureClkNetwork(mode);
  const sta::ClockSet* clock_set = mode->clkNetwork()->clocks(pin);

  if (clock_set != nullptr && !clock_set->empty()) {
    // Returns the first clock for the given iterm, TODO can we have more than
    // one clock driver?
    return *clock_set->begin();
  }

  return std::nullopt;
}

bool IsScanCell(const sta::dbNetwork* db_network,
                const sta::LibertyCell* liberty_cell)
{
  const sta::TestCell* test_cell = liberty_cell->testCell();
  if (test_cell) {
    return db_network->getLibertyScanIn(test_cell) != nullptr
           && db_network->getLibertyScanEnable(test_cell) != nullptr;
  }
  return false;
}

odb::Point GetITermLocation(odb::dbITerm* iterm, odb::dbInst* inst)
{
  if (iterm != nullptr) {
    int x, y;
    if (iterm->getAvgXY(&x, &y)) {
      return odb::Point(x, y);
    }
  }
  return inst->getOrigin();
}

odb::dbBTerm* CreateNewPort(odb::dbBlock* block,
                            const std::string& port_name,
                            utl::Logger* logger,
                            odb::dbNet* net,
                            odb::dbSigType sig_type)
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
    net->setSigType(sig_type);
  }
  auto port = odb::dbBTerm::create(net, port_name.c_str());
  if (port == nullptr) {
    logger->error(utl::DFT,
                  18,
                  "Failed to create port: a port named '{}' already exists",
                  port_name);
  }
  port->setSigType(sig_type);

  return port;
}

}  // namespace dft::utils

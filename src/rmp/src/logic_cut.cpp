// Copyright 2024 Google LLC
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include "logic_cut.h"

#include <unordered_map>
#include <vector>

#include "abc_library_factory.h"
#include "base/abc/abc.h"
#include "db_sta/dbNetwork.hh"
#include "map/mio/mio.h"
#include "sta/Liberty.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"

namespace rmp {

std::unordered_map<sta::Pin*, abc::Abc_Obj_t*> CreateAbcPrimaryInputs(
    const std::vector<sta::Pin*>& primary_inputs,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network)
{
  std::unordered_map<sta::Pin*, abc::Abc_Obj_t*> name_pin_map;
  for (sta::Pin* input_pin : primary_inputs) {
    abc::Abc_Obj_t* primary_input_abc = abc::Abc_NtkCreatePi(&abc_network);
    std::string pin_name(network->name(input_pin));
    abc::Abc_ObjAssignName(
        primary_input_abc, pin_name.data(), /*pSuffix=*/nullptr);
    name_pin_map[input_pin] = primary_input_abc;
  }

  return name_pin_map;
}

std::unordered_map<sta::Pin*, abc::Abc_Obj_t*> CreateAbcPrimaryOutputs(
    const std::vector<sta::Pin*>& primary_outputs,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network)
{
  std::unordered_map<sta::Pin*, abc::Abc_Obj_t*> name_pin_map;
  for (sta::Pin* output_pin : primary_outputs) {
    abc::Abc_Obj_t* primary_output_abc = abc::Abc_NtkCreatePo(&abc_network);
    std::string pin_name(network->name(output_pin));
    abc::Abc_ObjAssignName(
        primary_output_abc, pin_name.data(), /*pSuffix=*/nullptr);
    name_pin_map[output_pin] = primary_output_abc;
  }

  return name_pin_map;
}

std::unordered_map<std::string, abc::Mio_Gate_t*> NameToAbcGateMap(
    abc::Mio_Library_t* library)
{
  std::unordered_map<std::string, abc::Mio_Gate_t*> result;

  // Create map of cell names to Mio_Gate_t
  abc::Mio_Gate_t* current_gate = abc::Mio_LibraryReadGates(library);
  while (current_gate) {
    result[abc::Mio_GateReadName(current_gate)] = current_gate;
    current_gate = abc::Mio_GateReadNext(current_gate);
  }

  return result;
}

std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>>
MioGateToPortOrder(abc::Mio_Library_t* library)
{
  std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>> result;

  // Create map of cell names to Mio_Gate_t
  abc::Mio_Gate_t* current_gate = abc::Mio_LibraryReadGates(library);
  while (current_gate) {
    abc::Mio_Pin_t* current_pin = abc::Mio_GateReadPins(current_gate);
    std::vector<std::string> pin_order;
    while (current_pin) {
      pin_order.emplace_back(abc::Mio_PinReadName(current_pin));
      current_pin = abc::Mio_PinReadNext(current_pin);
    }
    result[current_gate] = std::move(pin_order);
    current_gate = abc::Mio_GateReadNext(current_gate);
  }

  return result;
}

std::unordered_map<sta::Instance*, abc::Abc_Obj_t*> CreateStandardCells(
    const std::unordered_set<sta::Instance*>& cut_instances,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network,
    abc::Mio_Library_t* library,
    utl::Logger* logger)
{
  std::unordered_map<std::string, abc::Mio_Gate_t*> cell_name_to_mio
      = NameToAbcGateMap(library);

  std::unordered_map<sta::Instance*, abc::Abc_Obj_t*> instance_map;
  for (sta::Instance* instance : cut_instances) {
    abc::Abc_Obj_t* abc_cell = abc::Abc_NtkCreateNode(&abc_network);

    // Assign this node it's standard cell. This is what makes this node
    // an AND gate or whatever.
    sta::LibertyCell* cell = network->libertyCell(instance);
    std::string cell_name = cell->name();
    if (cell_name_to_mio.find(cell_name) == cell_name_to_mio.end()) {
      logger->error(utl::RMP,
                    1001,
                    "cell: {} was not found in the ABC library, this is a bug.",
                    cell_name);
    }
    abc::Abc_ObjSetData(abc_cell, cell_name_to_mio.at(cell_name));

    std::string instance_name = network->name(instance);
    abc::Abc_ObjAssignName(abc_cell, instance_name.data(), /*pSuffix=*/nullptr);
    instance_map[instance] = abc_cell;
  }

  return instance_map;
}

void ConnectPinToDriver(
    sta::dbNetwork* network,
    sta::Pin* output_pin,
    const std::unordered_map<sta::Pin*, abc::Abc_Obj_t*>&
        abc_primary_output_pins,
    std::unordered_map<sta::Net*, abc::Abc_Obj_t*>& abc_net_map,
    utl::Logger* logger,
    abc::Abc_Ntk_t& abc_network,
    const std::unordered_map<sta::Instance*, abc::Abc_Obj_t*>& abc_instances)
{
  // The instance / pin that will recieve a singal from a driver.
  abc::Abc_Obj_t* abc_fanin_reciever;
  if (abc_primary_output_pins.find(output_pin)
      != abc_primary_output_pins.end()) {
    // If it's a primary output look it up in our special map.
    abc_fanin_reciever = abc_primary_output_pins.at(output_pin);
  } else {
    // It's not a primary output it should be an instance of some kind.
    sta::Instance* instance = network->instance(output_pin);
    if (abc_instances.find(instance) == abc_instances.end()) {
      logger->error(utl::RMP,
                    1018,
                    "bug: cannot find instance {} in abc instance map",
                    network->name(instance));
    }
    abc_fanin_reciever = abc_instances.at(instance);
  }

  sta::Net* net = network->net(output_pin);
  // If the net already exists just connect it
  if (abc_net_map.find(net) != abc_net_map.end()) {
    abc::Abc_ObjAddFanin(abc_fanin_reciever, abc_net_map.at(net));
    return;
  }

  sta::PinSet* drivers = network->drivers(net);
  if (drivers->size() != 1) {
    logger->error(utl::RMP,
                  1002,
                  "bug: output_pin: {} has the wrong number of drivers {}",
                  network->name(output_pin),
                  drivers->size());
  }

  // Find the instance that drives this primary output. Abc assumes
  // all gates are single output so all we need is a reference to this
  // instance to look it up in the Instance* -> Abc_Obj_t map.
  const sta::Pin* driver = *drivers->begin();
  sta::Instance* driver_instance = network->instance(driver);
  abc::Abc_Obj_t* abc_net = abc::Abc_NtkCreateNet(&abc_network);

  if (abc_instances.find(driver_instance) == abc_instances.end()) {
    logger->error(utl::RMP,
                  1003,
                  "bug: ABC version of instance {} not found.",
                  network->name(driver_instance));
  }
  abc::Abc_ObjAddFanin(abc_net, abc_instances.at(driver_instance));
  abc::Abc_ObjAddFanin(abc_fanin_reciever, abc_net);
  abc_net_map[net] = abc_net;
}

void CreateNets(
    const std::vector<sta::Pin*>& output_pins,
    const std::unordered_map<sta::Pin*, abc::Abc_Obj_t*>&
        abc_primary_input_pins,
    const std::unordered_map<sta::Pin*, abc::Abc_Obj_t*>&
        abc_primary_output_pins,
    const std::unordered_map<sta::Instance*, abc::Abc_Obj_t*>& abc_instances,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network,
    abc::Mio_Library_t* library,
    utl::Logger* logger)
{
  // Sometimes we might create a net that drives multiple pins.
  // Save them here so that the ConnectPinToDriver function can reuse
  // the already created net.
  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_net_map;

  // Connect primary outputs
  for (sta::Pin* output_pin : output_pins) {
    ConnectPinToDriver(network,
                       output_pin,
                       abc_primary_output_pins,
                       abc_net_map,
                       logger,
                       abc_network,
                       abc_instances);
  }

  // Connect instances to their drivers
  std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>> port_order
      = MioGateToPortOrder(library);
  for (auto& [sta_instance, abc_instance] : abc_instances) {
    abc::Mio_Gate_t* gate
        = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(abc_instance));
    if (port_order.find(gate) == port_order.end()) {
      logger->error(utl::RMP,
                    1007,
                    "bug: Can't find gate for {}",
                    network->name(sta_instance));
    }

    std::vector<std::string> port_order_vec = port_order.at(gate);
    for (const auto& port_name : port_order_vec) {
      sta::Pin* pin = network->findPin(sta_instance, port_name.c_str());

      // Deal with primary inputs
      if (abc_primary_input_pins.find(pin) != abc_primary_input_pins.end()) {
        abc::Abc_Obj_t* primary_input_pin = abc_primary_input_pins.at(pin);
        abc::Abc_Obj_t* abc_input_net = abc::Abc_NtkCreateNet(&abc_network);
        abc::Abc_ObjAddFanin(abc_input_net, primary_input_pin);
        abc::Abc_ObjAddFanin(abc_instance, abc_input_net);
        continue;
      }

      ConnectPinToDriver(network,
                         pin,
                         abc_primary_output_pins,
                         abc_net_map,
                         logger,
                         abc_network,
                         abc_instances);
    }
  }
}

utl::deleted_unique_ptr<abc::Abc_Ntk_t> LogicCut::BuildMappedAbcNetwork(
    AbcLibrary& abc_library,
    sta::dbNetwork* network,
    utl::Logger* logger)
{
  utl::deleted_unique_ptr<abc::Abc_Ntk_t> abc_network(
      abc::Abc_NtkAlloc(abc::Abc_NtkType_t::ABC_NTK_NETLIST,
                        abc::Abc_NtkFunc_t::ABC_FUNC_MAP,
                        /*fUseMemMan=*/1),
      &abc::Abc_NtkDelete);

  std::unordered_map<sta::Pin*, abc::Abc_Obj_t*> abc_input_pins
      = CreateAbcPrimaryInputs(primary_inputs_, *abc_network, network);
  std::unordered_map<sta::Pin*, abc::Abc_Obj_t*> abc_output_pins
      = CreateAbcPrimaryOutputs(primary_outputs_, *abc_network, network);

  // Create MIO standard cell library
  abc::Mio_Library_t* mio_library
      = abc::Abc_SclDeriveGenlibSimple(abc_library.abc_library());
  abc_network->pManFunc = mio_library;

  // Create cells from cut instances, get a map of the created nodes
  // keyed by the instance in the netlist.
  std::unordered_map<sta::Instance*, abc::Abc_Obj_t*> standard_cells
      = CreateStandardCells(
          cut_instances_, *abc_network, network, mio_library, logger);

  CreateNets(primary_outputs_,
             abc_input_pins,
             abc_output_pins,
             standard_cells,
             *abc_network,
             network,
             mio_library,
             logger);

  return abc_network;
}

}  // namespace rmp
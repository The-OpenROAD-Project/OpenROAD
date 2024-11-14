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

std::unordered_map<sta::Net*, abc::Abc_Obj_t*> CreateAbcPrimaryInputs(
    const std::vector<sta::Net*>& primary_inputs,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network)
{
  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> name_pin_map;
  for (sta::Net* input_pin : primary_inputs) {
    abc::Abc_Obj_t* primary_input_abc = abc::Abc_NtkCreatePi(&abc_network);
    abc::Abc_Obj_t* primary_input_net = abc::Abc_NtkCreateNet(&abc_network);
    std::string net_name(network->name(input_pin));
    abc::Abc_ObjAssignName(
        primary_input_abc, net_name.data(), /*pSuffix=*/nullptr);
    abc::Abc_ObjAddFanin(primary_input_net, primary_input_abc);

    name_pin_map[input_pin] = primary_input_net;
  }

  return name_pin_map;
}

std::unordered_map<sta::Net*, abc::Abc_Obj_t*> CreateAbcPrimaryOutputs(
    const std::vector<sta::Net*>& primary_outputs,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network)
{
  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> name_pin_map;
  for (sta::Net* output_pin : primary_outputs) {
    abc::Abc_Obj_t* primary_output_abc = abc::Abc_NtkCreatePo(&abc_network);
    abc::Abc_Obj_t* primary_output_net = abc::Abc_NtkCreateNet(&abc_network);
    std::string net_name(network->name(output_pin));
    abc::Abc_ObjAssignName(
        primary_output_net, net_name.data(), /*pSuffix=*/nullptr);
    abc::Abc_ObjAddFanin(primary_output_abc, primary_output_net);
    name_pin_map[output_pin] = primary_output_net;
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

    // Assign this node its standard cell. This is what makes this node
    // an AND gate or whatever.
    sta::LibertyCell* cell = network->libertyCell(instance);
    std::string cell_name = cell->name();
    if (cell_name_to_mio.find(cell_name) == cell_name_to_mio.end()) {
      logger->error(utl::RMP,
                    1001,
                    "cell: {} was not found in the ABC library. Please report "
                    "this internal error.",
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
    const std::unordered_map<sta::Net*, abc::Abc_Obj_t*>&
        abc_primary_output_pins,
    std::unordered_map<sta::Net*, abc::Abc_Obj_t*>& abc_net_map,
    utl::Logger* logger,
    abc::Abc_Ntk_t& abc_network,
    const std::unordered_map<sta::Instance*, abc::Abc_Obj_t*>& abc_instances)
{
  sta::Instance* instance = network->instance(output_pin);
  if (abc_instances.find(instance) == abc_instances.end()) {
    logger->error(utl::RMP,
                  1018,
                  "Cannot find instance {} in abc instance map. Please "
                  "report this internal error.",
                  network->name(instance));
  }
  // The instance / pin that will recieve a singal from a driver.
  abc::Abc_Obj_t* abc_fanin_reciever = abc_instances.at(instance);

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
                  "output_pin: {} has the wrong number of drivers {}. Please "
                  "report this internal error.",
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
                  "ABC version of instance {} not found. Please report this "
                  "internal error.",
                  network->name(driver_instance));
  }
  abc::Abc_ObjAddFanin(abc_net, abc_instances.at(driver_instance));
  abc::Abc_ObjAddFanin(abc_fanin_reciever, abc_net);
  abc_net_map[net] = abc_net;
}

void CreateNets(
    const std::vector<sta::Net*>& output_nets,
    const std::unordered_map<sta::Net*, abc::Abc_Obj_t*>&
        abc_primary_input_nets,
    const std::unordered_map<sta::Net*, abc::Abc_Obj_t*>&
        abc_primary_output_nets,
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

  for (auto& [sta_net, abc_instance] : abc_primary_input_nets) {
    abc_net_map[sta_net] = abc_instance;
  }

  // Connect primary outputs from net. Grab first driver pin,
  // should be just one. Then grab the instance of that pin. Then
  // connect the fanout of the abc instance to the primary output of
  // that net.
  for (sta::Net* output_net : output_nets) {
    sta::PinSet* pinset = network->drivers(output_net);
    // There should be exactly one pin.
    const sta::Pin* pin = *pinset->begin();
    sta::Instance* instance = network->instance(pin);
    abc::Abc_Obj_t* abc_driver_instance = abc_instances.at(instance);
    abc::Abc_Obj_t* primary_output = abc_primary_output_nets.at(output_net);
    abc::Abc_ObjAddFanin(primary_output, abc_driver_instance);
  }

  // ABC expects the inputs to particular gates to happen in a certain implict
  // order. Create a map from library cells to port order.
  std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>> port_order
      = MioGateToPortOrder(library);

  // Loop through all the other instances
  for (auto& [sta_instance, abc_instance] : abc_instances) {
    abc::Mio_Gate_t* gate
        = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(abc_instance));

    if (port_order.find(gate) == port_order.end()) {
      logger->error(
          utl::RMP,
          1007,
          "Can't find gate for {}. Please report this internal error.",
          network->name(sta_instance));
    }

    // Connect in ABC port order
    std::vector<std::string> port_order_vec = port_order.at(gate);
    for (const auto& port_name : port_order_vec) {
      sta::Pin* pin = network->findPin(sta_instance, port_name.c_str());
      ConnectPinToDriver(network,
                         pin,
                         abc_primary_output_nets,
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

  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_input_pins
      = CreateAbcPrimaryInputs(primary_inputs_, *abc_network, network);
  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_output_pins
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
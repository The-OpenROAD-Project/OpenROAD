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
#include "unique_name.h"
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
    std::string net_name(network->pathName(input_pin));
    abc::Abc_ObjAssignName(
        primary_input_net, net_name.data(), /*pSuffix=*/nullptr);
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
    std::string net_name(network->pathName(output_pin));
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
    abc_net_map[output_net] = primary_output;
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

utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> LogicCut::BuildMappedAbcNetwork(
    AbcLibrary& abc_library,
    sta::dbNetwork* network,
    utl::Logger* logger)
{
  utl::UniquePtrWithDeleter<abc::Abc_Ntk_t> abc_network(
      abc::Abc_NtkAlloc(abc::Abc_NtkType_t::ABC_NTK_NETLIST,
                        abc::Abc_NtkFunc_t::ABC_FUNC_MAP,
                        /*fUseMemMan=*/1),
      &abc::Abc_NtkDelete);

  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_input_nets
      = CreateAbcPrimaryInputs(primary_inputs_, *abc_network, network);
  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_output_nets
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
             abc_input_nets,
             abc_output_nets,
             standard_cells,
             *abc_network,
             network,
             mio_library,
             logger);

  return abc_network;
}

sta::Instance* GetLogicalParentInstance(
    const std::unordered_set<sta::Instance*>& cut_instances_,
    sta::dbNetwork* network,
    utl::Logger* logger)
{
  // In physical designs with hierarchy we need to figure out
  // the parent module in which instances should be placed.
  // For now lets just grab the first Instance* we see. RMP
  // shouldn't be doing anything across modules right now, but
  // if that changes I assume things will break.
  if (cut_instances_.empty()) {
    logger->error(utl::RMP, 1011, "Empty logic cuts are not allowed");
  }

  sta::Instance* instance = nullptr;
  for (sta::Instance* cut_instance : cut_instances_) {
    if (instance == nullptr) {
      instance = network->parent(cut_instance);
    }

    if (instance != network->parent(cut_instance)) {
      logger->error(utl::RMP,
                    1012,
                    "LogiCuts with multiple parent modules are not allowed.");
    }
  }

  return instance;
}

std::unordered_map<abc::Abc_Obj_t*, sta::Instance*> CreateInstances(
    abc::Abc_Ntk_t* abc_network,
    sta::dbNetwork* network,
    sta::Instance* parent_instance,
    UniqueName& unique_name,
    utl::Logger* logger)
{
  std::unordered_map<abc::Abc_Obj_t*, sta::Instance*> result;

  for (int i = 0; i < abc::Abc_NtkObjNumMax(abc_network); i++) {
    // ABC stores all objects as a list of objs we need to filter
    // to the ones that represent nodes/standard cells.
    abc::Abc_Obj_t* node_obj = abc::Abc_NtkObj(abc_network, i);
    if (node_obj == nullptr || !abc::Abc_ObjIsNode(node_obj)) {
      continue;
    }

    auto std_cell = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(node_obj));
    std::string std_cell_name = abc::Mio_GateReadName(std_cell);
    sta::LibertyCell* liberty_cell
        = network->findLibertyCell(std_cell_name.c_str());

    if (liberty_cell == nullptr) {
      logger->error(
          utl::RMP,
          1010,
          "Could not find cell name {}, please report this internal error",
          std_cell_name);
    }

    sta::Instance* new_instance = network->makeInstance(
        liberty_cell, unique_name.GetUniqueName().c_str(), parent_instance);

    result[node_obj] = new_instance;
  }
  return result;
}

std::unordered_map<abc::Abc_Obj_t*, sta::Net*> CreateNets(
    abc::Abc_Ntk_t* abc_network,
    sta::dbNetwork* network,
    sta::Instance* parent_instance,
    UniqueName& unique_name,
    utl::Logger* logger)
{
  std::unordered_map<abc::Abc_Obj_t*, sta::Net*> result;

  // Get primary input and output nets
  for (int i = 0; i < abc::Abc_NtkObjNumMax(abc_network); i++) {
    abc::Abc_Obj_t* node_obj = abc::Abc_NtkObj(abc_network, i);
    if (node_obj == nullptr) {
      continue;
    }

    abc::Abc_Obj_t* net_obj = nullptr;
    if (abc::Abc_ObjIsPo(node_obj)) {
      net_obj = abc::Abc_ObjFanin0(node_obj);
    } else if (abc::Abc_ObjIsPi(node_obj)) {
      net_obj = abc::Abc_ObjFanout0(node_obj);
    } else {
      continue;
    }

    if (!abc::Abc_ObjIsNet(net_obj)) {
      logger->error(utl::RMP,
                    1013,
                    "Primary input or output is not connected to an AIG PI/PO, "
                    "please report this internal error");
    }

    std::string net_name = abc::Abc_ObjName(net_obj);
    sta::Net* net = network->findNet(net_name.c_str());
    if (!net) {
      logger->error(utl::RMP, 1024, "Cannot find primary net {}", net_name);
    }
    result[net_obj] = net;
  }

  for (int i = 0; i < abc::Abc_NtkObjNumMax(abc_network); i++) {
    // ABC stores all objects as a list of objs we need to filter
    // to the ones that represent nodes/standard cells.
    abc::Abc_Obj_t* node_obj = abc::Abc_NtkObj(abc_network, i);
    if (node_obj == nullptr || !abc::Abc_ObjIsNet(node_obj)) {
      continue;
    }

    if (result.find(node_obj) != result.end()) {
      continue;
    }

    sta::Net* sta_net = network->makeNet(unique_name.GetUniqueName().c_str(),
                                         parent_instance);
    result[node_obj] = sta_net;
  }

  return result;
}

void DeleteExistingLogicCut(sta::dbNetwork* network,
                            std::vector<sta::Net*>& primary_inputs,
                            std::vector<sta::Net*>& primary_outputs,
                            std::unordered_set<sta::Instance*>& cut_instances,
                            utl::Logger* logger)
{
  // Delete nets that only belong to the cut set.
  std::unordered_set<sta::Net*> nets_to_be_deleted;
  std::unordered_set<sta::Net*> primary_input_or_output_nets;

  for (sta::Net* net : primary_inputs) {
    primary_input_or_output_nets.insert(net);
  }
  for (sta::Net* net : primary_outputs) {
    primary_input_or_output_nets.insert(net);
  }

  for (sta::Instance* instance : cut_instances) {
    auto pin_iterator = std::unique_ptr<sta::InstancePinIterator>(
        network->pinIterator(instance));
    while (pin_iterator->hasNext()) {
      sta::Pin* pin = pin_iterator->next();
      sta::Net* connected_net = network->net(pin);
      // If pin isn't a primary input or output add to deleted list. The only
      // way this can happen is if a net is only used within the cutset, and
      // in that case we want to delete it.
      if (primary_input_or_output_nets.find(connected_net)
          == primary_input_or_output_nets.end()) {
        nets_to_be_deleted.insert(connected_net);
      }
    }
  }

  for (sta::Instance* instance : cut_instances) {
    network->deleteInstance(instance);
  }

  for (sta::Net* net : nets_to_be_deleted) {
    network->deleteNet(net);
  }
}

void ConnectInstances(
    abc::Abc_Ntk_t* abc_network,
    sta::dbNetwork* network,
    const std::unordered_map<abc::Abc_Obj_t*, sta::Instance*>& new_instances,
    const std::unordered_map<abc::Abc_Obj_t*, sta::Net*>& new_nets,
    utl::Logger* logger)
{
  abc::Mio_Library_t* library
      = static_cast<abc::Mio_Library_t*>(abc_network->pManFunc);
  std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>>
      gate_to_port_order = MioGateToPortOrder(library);
  for (auto& [abc_obj, sta_instance] : new_instances) {
    auto std_cell = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(abc_obj));
    if (gate_to_port_order.find(std_cell) == gate_to_port_order.end()) {
      logger->error(utl::RMP,
                    1021,
                    "Cannot find abc gate port order {}, please report this "
                    "internal error",
                    abc::Mio_GateReadName(std_cell));
    }

    // Connect fan-ins for instance

    std::string gate_name = abc::Mio_GateReadName(std_cell);
    sta::LibertyCell* cell = network->findLibertyCell(gate_name.c_str());
    if (!cell) {
      logger->error(utl::RMP, 1014, "Cannot find cell {}", gate_name);
    }

    int i = 0;
    // ABC doesn't really have a concept of port names and liberty ports
    // rather there is an implict order between fan-ins and its standard cell,
    // loop through in that order, and connect the ports correctly based
    // on the abc fan-in.
    for (const std::string& port_name : gate_to_port_order.at(std_cell)) {
      sta::LibertyPort* port = cell->findLibertyPort(port_name.c_str());
      if (!port) {
        logger->error(
            utl::RMP, 1015, "Cannot find port  {}/{}", gate_name, port_name);
      }

      abc::Abc_Obj_t* net = abc::Abc_ObjFanin(abc_obj, i);
      if (!abc::Abc_ObjIsNet(net)) {
        logger->error(utl::RMP,
                      1016,
                      "Object is not a net in ABC netlist {}",
                      abc::Abc_ObjName(net));
      }

      if (new_nets.find(net) == new_nets.end()) {
        logger->error(utl::RMP,
                      1022,
                      "Could not find corresponding sta net for abc net: {}",
                      abc::Abc_ObjName(net));
      }

      sta::Net* sta_net = new_nets.at(net);

      network->connect(sta_instance, port, sta_net);
      i++;
    }

    // Connect the fan-out, in this case the only fan-out since ABC only
    // deals with single output gates.
    std::string output_port_name = abc::Mio_GateReadOutName(std_cell);
    sta::LibertyPort* output_port
        = cell->findLibertyPort(output_port_name.c_str());

    if (!output_port) {
      logger->error(utl::RMP,
                    1017,
                    "Cannot find port  {}/{}",
                    gate_name,
                    output_port_name);
    }
    abc::Abc_Obj_t* net = abc::Abc_ObjFanout0(abc_obj);
    if (!abc::Abc_ObjIsNet(net)) {
      logger->error(utl::RMP,
                    1019,
                    "Object is not a net in ABC netlist {}",
                    abc::Abc_ObjName(net));
    }

    if (new_nets.find(net) == new_nets.end()) {
      logger->error(utl::RMP,
                    1020,
                    "Could not find corresponding sta net for abc net: {}",
                    abc::Abc_ObjName(net));
    }

    sta::Net* sta_net = new_nets.at(net);

    network->connect(sta_instance, output_port, sta_net);
  }
}

void LogicCut::InsertMappedAbcNetwork(abc::Abc_Ntk_t* abc_network,
                                      sta::dbNetwork* network,
                                      UniqueName& unique_name,
                                      utl::Logger* logger)
{
  if (!abc::Abc_NtkHasMapping(abc_network)) {
    logger->error(
        utl::RMP,
        1008,
        "abc_network has no mapping, please report this internal error.");
  }

  if (!abc::Abc_NtkIsNetlist(abc_network)) {
    logger->error(
        utl::RMP,
        1009,
        "abc_network is not a netlist, please report this internal error.");
  }

  sta::Instance* parent_instance
      = GetLogicalParentInstance(cut_instances_, network, logger);
  std::unordered_map<abc::Abc_Obj_t*, sta::Instance*> abc_objs_to_instances
      = CreateInstances(
          abc_network, network, parent_instance, unique_name, logger);
  std::unordered_map<abc::Abc_Obj_t*, sta::Net*> abc_nets_to_sta_nets
      = CreateNets(abc_network, network, parent_instance, unique_name, logger);

  // Get rid of the old cut in preparation to connect the new ones.
  DeleteExistingLogicCut(
      network, primary_inputs_, primary_outputs_, cut_instances_, logger);

  // Connects the new instances to each other and to their primary inputs
  // and outputs.
  ConnectInstances(abc_network,
                   network,
                   abc_objs_to_instances,
                   abc_nets_to_sta_nets,
                   logger);

  // Final clean up to make this cut valid again. Replace the old cut instances
  // with the new ones. This should result in an equally valid LogicCut since
  // the PI/POs haven't changed just the junk inside.
  cut_instances_.clear();
  cut_instances_.reserve(abc_objs_to_instances.size());

  for (const auto& kv : abc_objs_to_instances) {
    cut_instances_.insert(kv.second);
  }
}

}  // namespace rmp

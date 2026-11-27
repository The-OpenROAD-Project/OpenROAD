// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2024-2025, The OpenROAD Authors

#include "cut/logic_cut.h"

#include <string.h>  // NOLINT(modernize-deprecated-headers,misc-include-cleaner): strdup()

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "base/abc/abc.h"
#include "cut/abc_library_factory.h"
#include "db_sta/dbNetwork.hh"
#include "map/mio/mio.h"
#include "map/scl/sclLib.h"
#include "misc/vec/vecPtr.h"
#include "sta/Liberty.hh"
#include "sta/NetworkClass.hh"
#include "sta/PortDirection.hh"
#include "utl/Logger.h"
#include "utl/deleter.h"
#include "utl/unique_name.h"

namespace cut {

namespace {

std::unordered_map<sta::Net*, abc::Abc_Obj_t*> CreateAbcPrimaryInputs(
    const std::vector<sta::Net*>& primary_inputs,
    abc::Abc_Ntk_t& abc_network,
    sta::dbNetwork* network)
{
  std::vector<sta::Net*> sorted_primary_inputs(primary_inputs.begin(),
                                               primary_inputs.end());
  std::ranges::sort(sorted_primary_inputs, [network](sta::Net* a, sta::Net* b) {
    return network->id(a) < network->id(b);
  });

  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> name_pin_map;
  for (sta::Net* input_pin : sorted_primary_inputs) {
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
  std::vector<sta::Net*> sorted_primary_outputs(primary_outputs.begin(),
                                                primary_outputs.end());

  std::ranges::sort(sorted_primary_outputs,
                    [network](sta::Net* a, sta::Net* b) {
                      return network->id(a) < network->id(b);
                    });

  for (sta::Net* output_pin : sorted_primary_outputs) {
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

// Builds a mapped ABC netlist that mirrors a LogicCut's instances and nets.
class AbcNetlistBuilder
{
 public:
  AbcNetlistBuilder(abc::Abc_Ntk_t& abc_network,
                    AbcLibrary& abc_library,
                    sta::dbNetwork* network,
                    utl::Logger* logger)
      : abc_network_(abc_network),
        abc_library_(abc_library),
        mio_library_(abc_library.mio_library()),
        network_(network),
        logger_(logger)
  {
  }

  void Build(const std::vector<sta::Net*>& primary_inputs,
             const std::vector<sta::Net*>& primary_outputs,
             const sta::InstanceSet& cut_instances);

 private:
  // Creates one ABC node per cut instance in abc_instances_.
  void CreateStandardCells(const sta::InstanceSet& cut_instances);
  // Drives each primary output from its cut instance.
  void ConnectPrimaryOutputs(
      const std::vector<sta::Net*>& output_nets,
      const std::unordered_map<sta::Net*, abc::Abc_Obj_t*>& abc_output_nets);
  // Connects the fan-ins of every cut instance, in ABC port order.
  void ConnectFanins();
  // Connects input_pin's ABC node to the ABC net driving it.
  void ConnectPinToDriver(sta::Pin* input_pin);

  abc::Abc_Ntk_t& abc_network_;
  AbcLibrary& abc_library_;
  abc::Mio_Library_t* mio_library_;
  sta::dbNetwork* network_;
  utl::Logger* logger_;
  std::unordered_map<const sta::Instance*, abc::Abc_Obj_t*> abc_instances_;
  // Nets created so far, which ConnectPinToDriver reuses when a net drives
  // several pins.
  std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_nets_;
};

void AbcNetlistBuilder::Build(const std::vector<sta::Net*>& primary_inputs,
                              const std::vector<sta::Net*>& primary_outputs,
                              const sta::InstanceSet& cut_instances)
{
  abc_nets_ = CreateAbcPrimaryInputs(primary_inputs, abc_network_, network_);
  const std::unordered_map<sta::Net*, abc::Abc_Obj_t*> abc_output_nets
      = CreateAbcPrimaryOutputs(primary_outputs, abc_network_, network_);

  abc_network_.pManFunc = mio_library_;

  CreateStandardCells(cut_instances);
  ConnectPrimaryOutputs(primary_outputs, abc_output_nets);
  ConnectFanins();
}

void AbcNetlistBuilder::CreateStandardCells(
    const sta::InstanceSet& cut_instances)
{
  const std::unordered_map<std::string, abc::Mio_Gate_t*> cell_name_to_mio
      = NameToAbcGateMap(mio_library_);

  for (const sta::Instance* instance : cut_instances) {
    // Assign this node its standard cell. This is what makes this node
    // an AND gate or whatever.
    const sta::LibertyCell* cell = network_->libertyCell(instance);
    std::string cell_name = cell->name();

    int connected_outputs = 0;
    std::unique_ptr<sta::InstancePinIterator> pin_iter(
        network_->pinIterator(instance));
    while (pin_iter->hasNext()) {
      sta::Pin* pin = pin_iter->next();

      if (!network_->net(pin) || !network_->direction(pin)->isOutput()) {
        continue;
      }

      if (++connected_outputs > 1) {
        logger_->error(
            utl::CUT,
            50,
            "Instance {} of type {} has more than one output connected. "
            "Multioutput cells with more than one output used are not "
            "supported.",
            network_->name(instance),
            cell_name);
      }

      const sta::LibertyPort* port = network_->libertyPort(pin);
      const std::string port_name = port->name();

      // Need to check for const cells since the Mio_Library_t* filters out
      // const cells.
      if (abc_library_.IsConst0Port(cell_name, port_name)) {
        cell_name = abc::Mio_LibraryReadConst0(mio_library_)->pName;
      }

      if (abc_library_.IsConst1Port(cell_name, port_name)) {
        cell_name = abc::Mio_LibraryReadConst1(mio_library_)->pName;
      }

      // Is a regular standard cell
      if (!cell_name_to_mio.contains(cell_name)) {
        logger_->error(
            utl::CUT,
            24,
            "cell: {} was not found in the ABC library. Please report "
            "this internal error.",
            cell_name);
      }
      abc::Abc_Obj_t* abc_cell = abc::Abc_NtkCreateNode(&abc_network_);
      abc::Abc_ObjSetData(abc_cell, cell_name_to_mio.at(cell_name));
      std::string instance_name = network_->name(instance);
      abc::Abc_ObjAssignName(
          abc_cell, instance_name.data(), /*pSuffix=*/nullptr);
      abc_instances_[instance] = abc_cell;
    }
  }
}

void AbcNetlistBuilder::ConnectPrimaryOutputs(
    const std::vector<sta::Net*>& output_nets,
    const std::unordered_map<sta::Net*, abc::Abc_Obj_t*>& abc_output_nets)
{
  std::vector<sta::Net*> sorted_net(output_nets.begin(), output_nets.end());
  std::ranges::sort(sorted_net, [this](const sta::Net* a, const sta::Net* b) {
    return network_->id(a) < network_->id(b);
  });
  // Connect primary outputs from net. Grab first driver pin,
  // should be just one. Then grab the instance of that pin. Then
  // connect the fanout of the abc instance to the primary output of
  // that net.
  for (sta::Net* output_net : sorted_net) {
    const sta::PinSet* pinset = network_->drivers(output_net);
    // There should be exactly one pin.
    const sta::Pin* pin = *pinset->begin();
    const sta::Instance* instance = network_->instance(pin);
    abc::Abc_Obj_t* abc_driver_instance = abc_instances_.at(instance);
    abc::Abc_Obj_t* primary_output = abc_output_nets.at(output_net);
    abc::Abc_ObjAddFanin(primary_output, abc_driver_instance);
    abc_nets_[output_net] = primary_output;
  }
}

void AbcNetlistBuilder::ConnectFanins()
{
  // ABC expects the inputs to particular gates to happen in a certain implict
  // order. Create a map from library cells to port order.
  const std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>>
      port_order = MioGateToPortOrder(mio_library_);

  // Sort instances for stability.
  std::vector<std::pair<const sta::Instance*, abc::Abc_Obj_t*>>
      sorted_instances(abc_instances_.begin(), abc_instances_.end());
  std::ranges::sort(
      sorted_instances,
      [this](const std::pair<const sta::Instance*, abc::Abc_Obj_t*>& a,
             const std::pair<const sta::Instance*, abc::Abc_Obj_t*>& b) {
        return network_->id(a.first) < network_->id(b.first);
      });

  for (const auto& [sta_instance, abc_instance] : sorted_instances) {
    abc::Mio_Gate_t* gate
        = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(abc_instance));

    if (!port_order.contains(gate)) {
      logger_->error(
          utl::CUT,
          28,
          "Can't find gate for {}. Please report this internal error.",
          network_->name(sta_instance));
    }

    // Connect in ABC port order
    for (const std::string& port_name : port_order.at(gate)) {
      ConnectPinToDriver(network_->findPin(sta_instance, port_name));
    }
  }
}

void AbcNetlistBuilder::ConnectPinToDriver(sta::Pin* input_pin)
{
  const sta::Instance* instance = network_->instance(input_pin);
  if (!abc_instances_.contains(instance)) {
    logger_->error(utl::CUT,
                   25,
                   "Cannot find instance {} in abc instance map. Please "
                   "report this internal error.",
                   network_->name(instance));
  }
  // The instance / pin that will recieve a singal from a driver.
  abc::Abc_Obj_t* abc_fanin_reciever = abc_instances_.at(instance);

  sta::Net* net = network_->net(input_pin);
  // If the net already exists just connect it
  if (abc_nets_.contains(net)) {
    abc::Abc_ObjAddFanin(abc_fanin_reciever, abc_nets_.at(net));
    return;
  }

  const sta::PinSet* drivers = network_->drivers(net);
  if (drivers->size() != 1) {
    logger_->error(utl::CUT,
                   26,
                   "output_pin: {} has the wrong number of drivers {}. Please "
                   "report this internal error.",
                   network_->name(input_pin),
                   drivers->size());
  }

  // Find the instance that drives this primary output. Abc assumes
  // all gates are single output so all we need is a reference to this
  // instance to look it up in the Instance* -> Abc_Obj_t map.
  const sta::Pin* driver = *drivers->begin();
  sta::Instance* driver_instance = network_->instance(driver);
  abc::Abc_Obj_t* abc_net = abc::Abc_NtkCreateNet(&abc_network_);

  if (network_->isTopInstance(driver_instance)
      || network_->libertyCell(driver_instance)->isSequential()) {
    abc::Abc_Obj_t* abc_input = abc::Abc_NtkCreatePi(&abc_network_);
    abc::Abc_ObjAddFanin(abc_net, abc_input);
    std::string driver_name = network_->name(driver);
    abc::Abc_ObjAssignName(abc_net, driver_name.data(), nullptr);
  } else if (!abc_instances_.contains(driver_instance)) {
    logger_->error(
        utl::CUT,
        27,
        "ABC version of instance {} of type {} not found. Please report this "
        "internal error.",
        network_->name(driver_instance),
        network_->libertyCell(driver_instance)->name());
  } else {
    abc::Abc_ObjAddFanin(abc_net, abc_instances_.at(driver_instance));
  }
  abc::Abc_ObjAddFanin(abc_fanin_reciever, abc_net);
  abc_nets_[net] = abc_net;
}

void AssertAbcNetworkHasNoZeroFanoutNodes(abc::Abc_Ntk_t* abc_network,
                                          utl::Logger* logger)
{
  for (int i = 0; i < abc::Vec_PtrSize(abc_network->vObjs); i++) {
    abc::Abc_Obj_t* obj = abc::Abc_NtkObj(abc_network, i);
    if (obj == nullptr || !abc::Abc_ObjIsNode(obj)) {
      continue;
    }

    if (abc::Abc_ObjFanoutNum(obj) == 0) {
      logger->error(utl::CUT,
                    29,
                    "Zero fanout node emitted from ABC. Please report this "
                    "internal error.");
    }
  }
}

sta::Instance* GetLogicalParentInstance(const sta::InstanceSet& cut_instances,
                                        sta::dbNetwork* network,
                                        utl::Logger* logger)
{
  // In physical designs with hierarchy we need to figure out
  // the parent module in which instances should be placed.
  // For now lets just grab the first Instance* we see. CUT
  // shouldn't be doing anything across modules right now, but
  // if that changes I assume things will break.
  if (cut_instances.empty()) {
    logger->error(utl::CUT, 30, "Empty logic cuts are not allowed");
  }

  sta::Instance* instance = nullptr;
  for (const sta::Instance* cut_instance : cut_instances) {
    if (instance == nullptr) {
      instance = network->parent(cut_instance);
    }

    if (instance != network->parent(cut_instance)) {
      logger->error(utl::CUT,
                    31,
                    "LogiCuts with multiple parent modules are not allowed.");
    }
  }

  return instance;
}

// Creates and connects the sta instances and nets for a mapped ABC netlist.
class StaNetlistInserter
{
 public:
  StaNetlistInserter(abc::Abc_Ntk_t* abc_network,
                     sta::dbNetwork* network,
                     sta::Instance* parent_instance,
                     utl::UniqueName& unique_name,
                     utl::Logger* logger)
      : abc_network_(abc_network),
        network_(network),
        parent_instance_(parent_instance),
        unique_name_(unique_name),
        logger_(logger)
  {
  }

  // Creates one sta instance per ABC node.
  void CreateInstances();
  // Maps ABC primary input/output nets to their existing sta nets and
  // creates sta nets for the internal ones.
  void CreateNets();
  // Connects the new instances to the new nets.
  void ConnectInstances();

  const std::unordered_map<abc::Abc_Obj_t*, sta::Instance*>& instances() const
  {
    return instances_;
  }

 private:
  void ConnectInstance(
      abc::Abc_Obj_t* abc_obj,
      sta::Instance* sta_instance,
      const std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>>&
          gate_to_port_order);

  abc::Abc_Ntk_t* abc_network_;
  sta::dbNetwork* network_;
  sta::Instance* parent_instance_;
  utl::UniqueName& unique_name_;
  utl::Logger* logger_;
  std::unordered_map<abc::Abc_Obj_t*, sta::Instance*> instances_;
  std::unordered_map<abc::Abc_Obj_t*, sta::Net*> nets_;
};

void StaNetlistInserter::CreateInstances()
{
  for (int i = 0; i < abc::Abc_NtkObjNumMax(abc_network_); i++) {
    // ABC stores all objects as a list of objs we need to filter
    // to the ones that represent nodes/standard cells.
    abc::Abc_Obj_t* node_obj = abc::Abc_NtkObj(abc_network_, i);
    if (node_obj == nullptr || !abc::Abc_ObjIsNode(node_obj)) {
      continue;
    }

    auto std_cell = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(node_obj));
    const std::string std_cell_name = abc::Mio_GateReadName(std_cell);
    sta::LibertyCell* liberty_cell = network_->findLibertyCell(std_cell_name);

    if (liberty_cell == nullptr) {
      logger_->error(
          utl::CUT,
          32,
          "Could not find cell name {}, please report this internal error",
          std_cell_name);
    }

    sta::Instance* new_instance = network_->makeInstance(
        liberty_cell, unique_name_.GetUniqueName("cut_"), parent_instance_);

    instances_[node_obj] = new_instance;
  }
}

void StaNetlistInserter::CreateNets()
{
  // Get primary input and output nets
  for (int i = 0; i < abc::Abc_NtkObjNumMax(abc_network_); i++) {
    abc::Abc_Obj_t* node_obj = abc::Abc_NtkObj(abc_network_, i);
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
      logger_->error(utl::CUT,
                     33,
                     "Primary input or output is not connected to an AIG "
                     "PI/PO, please report this internal error");
    }

    const std::string net_name = abc::Abc_ObjName(net_obj);
    sta::Net* net = network_->findNet(net_name);
    if (!net) {
      logger_->error(utl::CUT, 34, "Cannot find primary net {}", net_name);
    }
    nets_[net_obj] = net;
  }

  for (int i = 0; i < abc::Abc_NtkObjNumMax(abc_network_); i++) {
    // ABC stores all objects as a list of objs we need to filter
    // to the ones that represent nodes/standard cells.
    abc::Abc_Obj_t* node_obj = abc::Abc_NtkObj(abc_network_, i);
    if (node_obj == nullptr || !abc::Abc_ObjIsNet(node_obj)) {
      continue;
    }

    if (nets_.contains(node_obj)) {
      continue;
    }

    sta::Net* sta_net = network_->makeNet(unique_name_.GetUniqueName("cut_"),
                                          parent_instance_);
    nets_[node_obj] = sta_net;
  }
}

void StaNetlistInserter::ConnectInstances()
{
  auto library = static_cast<abc::Mio_Library_t*>(abc_network_->pManFunc);
  const std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>>
      gate_to_port_order = MioGateToPortOrder(library);

  // Sorted for stability
  std::vector<std::pair<abc::Abc_Obj_t*, sta::Instance*>> sorted_new_instances(
      instances_.begin(), instances_.end());
  std::ranges::sort(
      sorted_new_instances,
      [this](const std::pair<abc::Abc_Obj_t*, sta::Instance*>& a,
             const std::pair<abc::Abc_Obj_t*, sta::Instance*>& b) {
        return network_->id(a.second) < network_->id(b.second);
      });

  for (const auto& [abc_obj, sta_instance] : sorted_new_instances) {
    ConnectInstance(abc_obj, sta_instance, gate_to_port_order);
  }
}

void StaNetlistInserter::ConnectInstance(
    abc::Abc_Obj_t* abc_obj,
    sta::Instance* sta_instance,
    const std::unordered_map<abc::Mio_Gate_t*, std::vector<std::string>>&
        gate_to_port_order)
{
  auto std_cell = static_cast<abc::Mio_Gate_t*>(abc::Abc_ObjData(abc_obj));
  if (!gate_to_port_order.contains(std_cell)) {
    logger_->error(utl::CUT,
                   35,
                   "Cannot find abc gate port order {}, please report this "
                   "internal error",
                   abc::Mio_GateReadName(std_cell));
  }

  // Connect fan-ins for instance
  const std::string gate_name = abc::Mio_GateReadName(std_cell);
  sta::LibertyCell* cell = network_->findLibertyCell(gate_name);
  if (!cell) {
    logger_->error(utl::CUT, 36, "Cannot find cell {}", gate_name);
  }

  int i = 0;
  // ABC doesn't really have a concept of port names and liberty ports
  // rather there is an implict order between fan-ins and its standard cell,
  // loop through in that order, and connect the ports correctly based
  // on the abc fan-in.
  for (const std::string& port_name : gate_to_port_order.at(std_cell)) {
    sta::LibertyPort* port = cell->findLibertyPort(port_name);
    if (!port) {
      logger_->error(
          utl::CUT, 37, "Cannot find port  {}/{}", gate_name, port_name);
    }

    abc::Abc_Obj_t* net = abc::Abc_ObjFanin(abc_obj, i);
    if (!abc::Abc_ObjIsNet(net)) {
      logger_->error(utl::CUT,
                     38,
                     "Object is not a net in ABC netlist {}",
                     abc::Abc_ObjName(net));
    }

    if (!nets_.contains(net)) {
      logger_->error(utl::CUT,
                     39,
                     "Could not find corresponding sta net for abc net: {}",
                     abc::Abc_ObjName(net));
    }

    network_->connect(sta_instance, port, nets_.at(net));
    i++;
  }

  // Connect the fan-out, in this case the only fan-out since ABC only
  // deals with single output gates.
  const std::string output_port_name = abc::Mio_GateReadOutName(std_cell);
  sta::LibertyPort* output_port = cell->findLibertyPort(output_port_name);

  if (!output_port) {
    logger_->error(
        utl::CUT, 40, "Cannot find port  {}/{}", gate_name, output_port_name);
  }
  abc::Abc_Obj_t* net = abc::Abc_ObjFanout0(abc_obj);
  if (!abc::Abc_ObjIsNet(net)) {
    logger_->error(utl::CUT,
                   41,
                   "Object is not a net in ABC netlist {}",
                   abc::Abc_ObjName(net));
  }

  if (!nets_.contains(net)) {
    logger_->error(utl::CUT,
                   42,
                   "Could not find corresponding sta net for abc net: {}",
                   abc::Abc_ObjName(net));
  }

  network_->connect(sta_instance, output_port, nets_.at(net));
}

void DeleteExistingLogicCut(sta::dbNetwork* network,
                            std::vector<sta::Net*>& primary_inputs,
                            std::vector<sta::Net*>& primary_outputs,
                            sta::InstanceSet& cut_instances)
{
  // Delete nets that only belong to the cut set.
  sta::NetSet nets_to_be_deleted(network);
  std::unordered_set<sta::Net*> primary_input_or_output_nets;

  for (sta::Net* net : primary_inputs) {
    primary_input_or_output_nets.insert(net);
  }
  for (sta::Net* net : primary_outputs) {
    primary_input_or_output_nets.insert(net);
  }

  for (const sta::Instance* instance : cut_instances) {
    auto pin_iterator = std::unique_ptr<sta::InstancePinIterator>(
        network->pinIterator(instance));
    while (pin_iterator->hasNext()) {
      sta::Pin* pin = pin_iterator->next();
      sta::Net* connected_net = network->net(pin);
      if (connected_net == nullptr) {
        // This net is not connected to anything, so we cannot delete it.
        // This can happen if you have an unconnected output port.
        // For example one of Sky130's tie cell has both high and low outputs
        // and only one is connected to a net.
        continue;
      }
      // If pin isn't a primary input or output add to deleted list. The only
      // way this can happen is if a net is only used within the cutset, and
      // in that case we want to delete it.
      if (!primary_input_or_output_nets.contains(connected_net)) {
        nets_to_be_deleted.insert(connected_net);
      }
    }
  }
  for (const sta::Instance* instance : cut_instances) {
    network->deleteInstance(const_cast<sta::Instance*>(instance));
  }

  for (const sta::Net* net : nets_to_be_deleted) {
    network->deleteNet(const_cast<sta::Net*>(net));
  }
}

void MapConstantCells(AbcLibrary& abc_library,
                      abc::Abc_Ntk_t* abc_network,
                      utl::Logger* logger)
{
  std::pair<abc::SC_Cell*, abc::SC_Pin*> const_0
      = abc_library.ConstantZeroCell();
  if (!const_0.first || !const_0.second) {
    logger->error(utl::CUT, 43, "cannot find 0 valued constant tie cell.");
  }

  std::pair<abc::SC_Cell*, abc::SC_Pin*> const_1
      = abc_library.ConstantOneCell();
  if (!const_1.first || !const_1.second) {
    logger->error(utl::CUT, 44, "cannot find 1 valued constant tie cell.");
  }
  std::string const_0_output_pin_name = const_0.second->pName;
  std::string const_1_output_pin_name = const_1.second->pName;

  auto library = static_cast<abc::Mio_Library_t*>(abc_network->pManFunc);

  // Map 0 cell
  abc::Mio_Gate_t* zero_cell = abc::Mio_LibraryReadConst0(library);
  free(zero_cell->pName);
  free(zero_cell->pOutName);
  zero_cell->pName = strdup(const_0.first->pName);
  zero_cell->pOutName = strdup(const_0_output_pin_name.c_str());

  abc::Mio_Gate_t* one_cell = abc::Mio_LibraryReadConst1(library);
  free(one_cell->pName);
  free(one_cell->pOutName);
  one_cell->pName = strdup(const_1.first->pName);
  one_cell->pOutName = strdup(const_1_output_pin_name.c_str());
}

}  // namespace

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

  AbcNetlistBuilder builder(*abc_network, abc_library, network, logger);
  builder.Build(primary_inputs_, primary_outputs_, cut_instances_);

  AssertAbcNetworkHasNoZeroFanoutNodes(abc_network.get(), logger);

  return abc_network;
}

void LogicCut::InsertMappedAbcNetwork(abc::Abc_Ntk_t* abc_network,
                                      AbcLibrary& abc_library,
                                      sta::dbNetwork* network,
                                      utl::UniqueName& unique_name,
                                      utl::Logger* logger)
{
  if (!abc::Abc_NtkHasMapping(abc_network)) {
    logger->error(
        utl::CUT,
        45,
        "abc_network has no mapping, please report this internal error.");
  }

  if (!abc::Abc_NtkIsNetlist(abc_network)) {
    logger->error(
        utl::CUT,
        46,
        "abc_network is not a netlist, please report this internal error.");
  }

  MapConstantCells(abc_library, abc_network, logger);

  sta::Instance* parent_instance
      = GetLogicalParentInstance(cut_instances_, network, logger);
  StaNetlistInserter inserter(
      abc_network, network, parent_instance, unique_name, logger);
  inserter.CreateInstances();
  inserter.CreateNets();

  // Get rid of the old cut in preparation to connect the new ones.
  DeleteExistingLogicCut(
      network, primary_inputs_, primary_outputs_, cut_instances_);

  // Connects the new instances to each other and to their primary inputs
  // and outputs.
  inserter.ConnectInstances();

  // Final clean up to make this cut valid again. Replace the old cut instances
  // with the new ones. This should result in an equally valid LogicCut since
  // the PI/POs haven't changed just the junk inside.
  cut_instances_.clear();
  for (const auto& kv : inserter.instances()) {
    cut_instances_.insert(kv.second);
  }
}

}  // namespace cut

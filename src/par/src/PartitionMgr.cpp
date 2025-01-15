/////////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////////

#include "par/PartitionMgr.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include "TritonPart.h"
#include "db_sta/dbSta.hh"
#include "odb/db.h"
#include "sta/ConcreteNetwork.hh"
#include "sta/Liberty.hh"
#include "sta/MakeConcreteNetwork.hh"
#include "sta/NetworkClass.hh"
#include "sta/ParseBus.hh"
#include "sta/PortDirection.hh"
#include "sta/VerilogWriter.hh"
#include "utl/Logger.h"

using odb::dbBlock;
using odb::dbInst;
using odb::dbIntProperty;

using sta::Cell;
using sta::CellPortBitIterator;
using sta::ConcreteNetwork;
using sta::dbNetwork;
using sta::Instance;
using sta::InstancePinIterator;
using sta::isBusName;
using sta::Library;
using sta::Net;
using sta::NetPinIterator;
using sta::NetTermIterator;
using sta::NetworkReader;
using sta::parseBusName;
using sta::Pin;
using sta::Port;
using sta::PortDirection;
using sta::Term;
using sta::writeVerilog;
using utl::PAR;

namespace par {

bool CompareInstancePtr::operator()(const sta::Instance* lhs,
                                    const sta::Instance* rhs) const
{
  return db_network_->staToDb(lhs)->getName()
         < db_network_->staToDb(rhs)->getName();
}

void PartitionMgr::init(odb::dbDatabase* db,
                        sta::dbNetwork* db_network,
                        sta::dbSta* sta,
                        utl::Logger* logger)
{
  db_ = db;
  db_network_ = db_network;
  sta_ = sta;
  logger_ = logger;
}

// The function for partitioning a hypergraph
// This is used for replacing hMETIS
// Key supports:
// (1) fixed vertices constraint in fixed_file
// (2) community attributes in community_file (This can be used to guide the
// partitioning process) (3) stay together attributes in group_file. (4)
// placement information is specified in placement file The format is that each
// line cooresponds to a group fixed vertices, community and placement
// attributes both follows the hMETIS format
void PartitionMgr::tritonPartHypergraph(
    unsigned int num_parts,
    float balance_constraint,
    const std::vector<float>& base_balance,
    const std::vector<float>& scale_factor,
    unsigned int seed,
    int vertex_dimension,
    int hyperedge_dimension,
    int placement_dimension,
    const char* hypergraph_file,
    const char* fixed_file,
    const char* community_file,
    const char* group_file,
    const char* placement_file,
    // weight parameters
    const std::vector<float>& e_wt_factors,
    const std::vector<float>& v_wt_factors,
    const std::vector<float>& placement_wt_factors,
    // coarsening related parameters
    int thr_coarsen_hyperedge_size_skip,
    int thr_coarsen_vertices,
    int thr_coarsen_hyperedges,
    float coarsening_ratio,
    int max_coarsen_iters,
    float adj_diff_ratio,
    int min_num_vertices_each_part,
    // initial partitioning related parameters
    int num_initial_solutions,
    int num_best_initial_solutions,
    // refinement related parameters
    int refiner_iters,
    int max_moves,
    float early_stop_ratio,
    int total_corking_passes,
    // vcycle related parameters
    bool v_cycle_flag,
    int max_num_vcycle,
    int num_coarsen_solutions,
    int num_vertices_threshold_ilp,
    int global_net_threshold)
{
  // Use TritonPart to partition a hypergraph
  // In this mode, TritonPart works as hMETIS.
  // Thus users can use this function to partition the input hypergraph
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, sta_, logger_);
  // Convert the string e_wt_factors_str to vector
  triton_part->SetNetWeight(e_wt_factors);
  triton_part->SetVertexWeight(v_wt_factors);
  triton_part->SetPlacementWeight(placement_wt_factors);
  triton_part->SetFineTuneParams(  // coarsening related parameters
      thr_coarsen_hyperedge_size_skip,
      thr_coarsen_vertices,
      thr_coarsen_hyperedges,
      coarsening_ratio,
      max_coarsen_iters,
      adj_diff_ratio,
      min_num_vertices_each_part,
      // initial partitioning related parameters
      num_initial_solutions,
      num_best_initial_solutions,
      // refinement related parameters
      refiner_iters,
      max_moves,
      early_stop_ratio,
      total_corking_passes,
      // vcycle related parameters
      v_cycle_flag,
      max_num_vcycle,
      num_coarsen_solutions,
      num_vertices_threshold_ilp,
      global_net_threshold);

  triton_part->PartitionHypergraph(num_parts,
                                   balance_constraint,
                                   base_balance,
                                   scale_factor,
                                   seed,
                                   vertex_dimension,
                                   hyperedge_dimension,
                                   placement_dimension,
                                   hypergraph_file,
                                   fixed_file,
                                   community_file,
                                   group_file,
                                   placement_file);
}

// Evaluate a given solution of a hypergraph
// The fixed vertices should statisfy the fixed vertices constraint
// The group of vertices should stay together in the solution
// The vertex balance should be satisfied
void PartitionMgr::evaluateHypergraphSolution(
    unsigned int num_parts,
    float balance_constraint,
    const std::vector<float>& base_balance,
    const std::vector<float>& scale_factor,
    int vertex_dimension,
    int hyperedge_dimension,
    const char* hypergraph_file,
    const char* fixed_file,
    const char* group_file,
    const char* solution_file,
    // weight parameters
    const std::vector<float>& e_wt_factors,
    const std::vector<float>& v_wt_factors)
{
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, sta_, logger_);
  // Convert the string e_wt_factors_str to vector
  triton_part->SetNetWeight(e_wt_factors);
  triton_part->SetVertexWeight(v_wt_factors);
  triton_part->EvaluateHypergraphSolution(num_parts,
                                          balance_constraint,
                                          base_balance,
                                          scale_factor,
                                          vertex_dimension,
                                          hyperedge_dimension,
                                          hypergraph_file,
                                          fixed_file,
                                          group_file,
                                          solution_file);
}

// Top level interface
// The function for partitioning a netlist
// This is the main API for TritonPart
// Key supports:
// (1) fixed vertices constraint in fixed_file
// (2) community attributes in community_file (This can be used to guide the
// partitioning process) (3) stay together attributes in group_file. (4)
// timing-driven partitioning (5) fence-aware partitioning (6) placement-aware
// partitioning, placement information is extracted from OpenDB
void PartitionMgr::tritonPartDesign(
    unsigned int num_parts_arg,
    float balance_constraint_arg,
    const std::vector<float>& base_balance_arg,
    const std::vector<float>& scale_factor_arg,
    unsigned int seed_arg,
    bool timing_aware_flag_arg,
    int top_n_arg,
    bool placement_flag_arg,
    bool fence_flag_arg,
    float fence_lx_arg,
    float fence_ly_arg,
    float fence_ux_arg,
    float fence_uy_arg,
    const char* fixed_file_arg,
    const char* community_file_arg,
    const char* group_file_arg,
    const char* solution_filename_arg,
    // timing related parameters
    float net_timing_factor,
    float path_timing_factor,
    float path_snaking_factor,
    float timing_exp_factor,
    float extra_delay,
    bool guardband_flag,
    // weight parameters
    const std::vector<float>& e_wt_factors,
    const std::vector<float>& v_wt_factors,
    const std::vector<float>& placement_wt_factors,
    // coarsening related parameters
    int thr_coarsen_hyperedge_size_skip,
    int thr_coarsen_vertices,
    int thr_coarsen_hyperedges,
    float coarsening_ratio,
    int max_coarsen_iters,
    float adj_diff_ratio,
    int min_num_vertices_each_part,
    // initial partitioning related parameters
    int num_initial_solutions,
    int num_best_initial_solutions,
    // refinement related parameters
    int refiner_iters,
    int max_moves,
    float early_stop_ratio,
    int total_corking_passes,
    // vcycle related parameters
    bool v_cycle_flag,
    int max_num_vcycle,
    int num_coarsen_solutions,
    int num_vertices_threshold_ilp,
    int global_net_threshold)
{
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, sta_, logger_);
  // Convert the string e_wt_factors_str to vector
  triton_part->SetNetWeight(e_wt_factors);
  triton_part->SetVertexWeight(v_wt_factors);
  triton_part->SetPlacementWeight(placement_wt_factors);
  triton_part->SetTimingParams(net_timing_factor,
                               path_timing_factor,
                               path_snaking_factor,
                               timing_exp_factor,
                               extra_delay,
                               guardband_flag);
  triton_part->SetFineTuneParams(  // coarsening related parameters
      thr_coarsen_hyperedge_size_skip,
      thr_coarsen_vertices,
      thr_coarsen_hyperedges,
      coarsening_ratio,
      max_coarsen_iters,
      adj_diff_ratio,
      min_num_vertices_each_part,
      // initial partitioning related parameters
      num_initial_solutions,
      num_best_initial_solutions,
      // refinement related parameters
      refiner_iters,
      max_moves,
      early_stop_ratio,
      total_corking_passes,
      // vcycle related parameters
      v_cycle_flag,
      max_num_vcycle,
      num_coarsen_solutions,
      num_vertices_threshold_ilp,
      global_net_threshold);

  triton_part->PartitionDesign(num_parts_arg,
                               balance_constraint_arg,
                               base_balance_arg,
                               scale_factor_arg,
                               seed_arg,
                               timing_aware_flag_arg,
                               top_n_arg,
                               placement_flag_arg,
                               fence_flag_arg,
                               fence_lx_arg,
                               fence_ly_arg,
                               fence_ux_arg,
                               fence_uy_arg,
                               fixed_file_arg,
                               community_file_arg,
                               group_file_arg,
                               solution_filename_arg);
}

// Function to evaluate the hypergraph partitioning solution
// This can be used to write the timing-weighted hypergraph
// and evaluate the solution.
// If the solution file is empty, then this function is to write the
// solution.
// If the solution file is not empty, then this function is to evaluate
// the solution without writing the hypergraph again
// This function is only used for testing
void PartitionMgr::evaluatePartDesignSolution(
    unsigned int num_parts_arg,
    float balance_constraint_arg,
    const std::vector<float>& base_balance_arg,
    const std::vector<float>& scale_factor_arg,
    bool timing_aware_flag_arg,
    int top_n_arg,
    bool fence_flag_arg,
    float fence_lx_arg,
    float fence_ly_arg,
    float fence_ux_arg,
    float fence_uy_arg,
    const char* fixed_file_arg,
    const char* community_file_arg,
    const char* group_file_arg,
    const char* hypergraph_file_arg,
    const char* hypergraph_int_weight_file_arg,
    const char* solution_filename_arg,
    // timing related parameters
    float net_timing_factor,
    float path_timing_factor,
    float path_snaking_factor,
    float timing_exp_factor,
    float extra_delay,
    bool guardband_flag,
    // weight parameters
    const std::vector<float>& e_wt_factors,
    const std::vector<float>& v_wt_factors)
{
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, sta_, logger_);
  // Convert the string e_wt_factors_str to vector
  triton_part->SetNetWeight(e_wt_factors);
  triton_part->SetVertexWeight(v_wt_factors);
  std::vector<float> placement_wt_factors;
  triton_part->SetPlacementWeight(placement_wt_factors);
  triton_part->SetTimingParams(net_timing_factor,
                               path_timing_factor,
                               path_snaking_factor,
                               timing_exp_factor,
                               extra_delay,
                               guardband_flag);

  triton_part->EvaluatePartDesignSolution(num_parts_arg,
                                          balance_constraint_arg,
                                          base_balance_arg,
                                          scale_factor_arg,
                                          timing_aware_flag_arg,
                                          top_n_arg,
                                          fence_flag_arg,
                                          fence_lx_arg,
                                          fence_ly_arg,
                                          fence_ux_arg,
                                          fence_uy_arg,
                                          fixed_file_arg,
                                          community_file_arg,
                                          group_file_arg,
                                          hypergraph_file_arg,
                                          hypergraph_int_weight_file_arg,
                                          solution_filename_arg);
}

// k-way partitioning used by Hier-RTLMP
std::vector<int> PartitionMgr::PartitionKWaySimpleMode(
    unsigned int num_parts_arg,
    float balance_constraint_arg,
    unsigned int seed_arg,
    const std::vector<std::vector<int>>& hyperedges,
    const std::vector<float>& vertex_weights,
    const std::vector<float>& hyperedge_weights)
{
  auto triton_part
      = std::make_unique<TritonPart>(db_network_, db_, sta_, logger_);
  return triton_part->PartitionKWaySimpleMode(num_parts_arg,
                                              balance_constraint_arg,
                                              seed_arg,
                                              hyperedges,
                                              vertex_weights,
                                              hyperedge_weights);
}

// determine the required direction of a port.
static PortDirection* determinePortDirection(
    const Net* net,
    const std::set<Instance*, CompareInstancePtr>* insts,
    const dbNetwork* db_network)
{
  bool local_only = true;
  bool locally_driven = false;
  bool externally_driven = false;

  NetTermIterator* term_iter = db_network->termIterator(net);
  while (term_iter->hasNext()) {
    Term* term = term_iter->next();
    PortDirection* dir = db_network->direction(db_network->pin(term));
    if (dir->isAnyInput()) {
      externally_driven = true;
    }
    local_only = false;
  }
  delete term_iter;

  if (insts != nullptr) {
    NetPinIterator* pin_iter = db_network->pinIterator(net);
    while (pin_iter->hasNext()) {
      const Pin* pin = pin_iter->next();
      const PortDirection* dir = db_network->direction(pin);
      Instance* inst = db_network->instance(pin);

      if (insts->find(inst) == insts->end()) {
        local_only = false;
        if (dir->isAnyOutput()) {
          externally_driven = true;
        }
      } else {
        if (dir->isAnyOutput()) {
          locally_driven = true;
        }
      }
    }
    delete pin_iter;
  }

  // no port is needed
  if (local_only) {
    return nullptr;
  }

  if (locally_driven && externally_driven) {
    return PortDirection::bidirect();
  }

  if (externally_driven) {
    return PortDirection::input();
  }

  return PortDirection::output();
}

// find the correct brackets used in the liberty libraries.
static void determineLibraryBrackets(const dbNetwork* db_network,
                                     char* left,
                                     char* right)
{
  *left = '[';
  *right = ']';

  sta::LibertyLibraryIterator* lib_iter = db_network->libertyLibraryIterator();
  while (lib_iter->hasNext()) {
    const sta::LibertyLibrary* lib = lib_iter->next();
    *left = lib->busBrktLeft();
    *right = lib->busBrktRight();
  }
  delete lib_iter;
}

// Builds an instance/cell of a partition
Instance* PartitionMgr::buildPartitionedInstance(
    const char* name,
    const char* port_prefix,
    sta::Library* library,
    sta::NetworkReader* network,
    sta::Instance* parent,
    const std::set<Instance*, CompareInstancePtr>* insts,
    std::map<Net*, Port*>* port_map)
{
  // build cell
  Cell* cell = network->makeCell(library, name, false, nullptr);

  // add global ports
  auto pin_iter = db_network_->pinIterator(db_network_->topInstance());
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();

    bool add_port = false;
    Net* net = db_network_->net(db_network_->term(pin));
    if (net) {
      NetPinIterator* net_pin_iter = db_network_->pinIterator(net);
      while (net_pin_iter->hasNext()) {
        // check if port is connected to instance in this partition
        if (insts->find(db_network_->instance(net_pin_iter->next()))
            != insts->end()) {
          add_port = true;
          break;
        }
      }
      delete net_pin_iter;
    }

    if (add_port) {
      const char* portname = db_network_->name(pin);

      Port* port = network->makePort(cell, portname);
      // copy exactly the parent port direction
      network->setDirection(port, db_network_->direction(pin));
      PortDirection* sub_module_dir
          = determinePortDirection(net, insts, db_network_);
      if (sub_module_dir != nullptr) {
        network->setDirection(port, sub_module_dir);
      }

      port_map->insert({net, port});
    }
  }
  delete pin_iter;

  // make internal ports for partitions and if port is not needed.
  std::set<Net*> local_nets;
  for (Instance* inst : *insts) {
    InstancePinIterator* pin_iter = db_network_->pinIterator(inst);
    while (pin_iter->hasNext()) {
      Net* net = db_network_->net(pin_iter->next());
      if (net != nullptr &&                          // connected
          port_map->find(net) == port_map->end() &&  // port not present
          local_nets.find(net) == local_nets.end()) {
        // check if connected to anything in a different partition
        NetPinIterator* net_pin_iter = db_network_->pinIterator(net);
        while (net_pin_iter->hasNext()) {
          Net* net = db_network_->net(net_pin_iter->next());
          PortDirection* port_dir
              = determinePortDirection(net, insts, db_network_);
          if (port_dir == nullptr) {
            local_nets.insert(net);
            continue;
          }
          std::string port_name = port_prefix;
          port_name += db_network_->name(net);

          Port* port = network->makePort(cell, port_name.c_str());
          network->setDirection(port, port_dir);

          port_map->insert({net, port});
          break;
        }
        delete net_pin_iter;
      }
    }
    delete pin_iter;
  }

  // loop over buses and to ensure all bit ports are created, only needed for
  // partitioned modules
  char path_escape = db_network_->pathEscape();
  char left_bracket;
  char right_bracket;
  determineLibraryBrackets(db_network_, &left_bracket, &right_bracket);
  std::map<std::string, std::vector<Port*>> port_buses;
  for (auto& [net, port] : *port_map) {
    std::string portname = network->name(port);

    // check if bus and get name
    if (isBusName(portname.c_str(), left_bracket, right_bracket, path_escape)) {
      std::string bus_name;
      bool is_bus;
      int idx;
      parseBusName(portname.c_str(),
                   left_bracket,
                   right_bracket,
                   path_escape,
                   is_bus,
                   bus_name,
                   idx);

      portname = bus_name;
      port_buses[portname].push_back(port);
    }
  }
  for (auto& [bus, ports] : port_buses) {
    std::set<int> port_idx;
    std::set<PortDirection*> port_dirs;
    for (Port* port : ports) {
      std::string bus_name;
      bool is_bus;
      int idx;
      parseBusName(network->name(port),
                   left_bracket,
                   right_bracket,
                   path_escape,
                   is_bus,
                   bus_name,
                   idx);

      port_idx.insert(idx);
      port_dirs.insert(network->direction(port));
    }

    // determine real direction of port
    PortDirection* overall_direction = nullptr;
    if (port_dirs.size() == 1) {  // only one direction is used.
      overall_direction = *port_dirs.begin();
    } else {
      overall_direction = PortDirection::bidirect();
    }

    // set port direction to match
    for (Port* port : ports) {
      network->setDirection(port, overall_direction);
    }

    // fill in missing ports in bus
    const auto [min_idx, max_idx]
        = std::minmax_element(port_idx.begin(), port_idx.end());
    for (int idx = *min_idx; idx <= *max_idx; idx++) {
      if (port_idx.find(idx) == port_idx.end()) {
        // build missing port
        std::string portname = bus;
        portname += left_bracket + std::to_string(idx) + right_bracket;
        Port* port = network->makePort(cell, portname.c_str());
        network->setDirection(port, overall_direction);
      }
    }
  }

  network->groupBusPorts(cell, [](const char*) { return true; });

  // build instance
  std::string instname = name;
  instname += "_inst";
  Instance* inst = network->makeInstance(cell, instname.c_str(), parent);

  // create nets for ports in cell
  for (auto& [db_net, port] : *port_map) {
    Net* net = network->makeNet(network->name(port), inst);
    Pin* pin = network->makePin(inst, port, nullptr);
    network->makeTerm(pin, net);
  }

  // create and connect instances
  for (Instance* instance : *insts) {
    Instance* leaf_inst = network->makeInstance(
        db_network_->cell(instance), db_network_->name(instance), inst);

    InstancePinIterator* pin_iter = db_network_->pinIterator(instance);
    while (pin_iter->hasNext()) {
      Pin* pin = pin_iter->next();
      Net* net = db_network_->net(pin);
      if (net != nullptr) {  // connected
        Port* port = db_network_->port(pin);

        // check if connected to a port
        auto port_find = port_map->find(net);
        if (port_find != port_map->end()) {
          Net* new_net
              = network->findNet(inst, network->name(port_find->second));
          network->connect(leaf_inst, port, new_net);
        } else {
          Net* new_net = network->findNet(inst, db_network_->name(net));
          if (new_net == nullptr) {
            new_net = network->makeNet(db_network_->name(net), inst);
          }
          network->connect(leaf_inst, port, new_net);
        }
      }
    }
    delete pin_iter;
  }

  return inst;
}

// Builds an instance/cell instantiating the partitions
Instance* PartitionMgr::buildPartitionedTopInstance(const char* name,
                                                    sta::Library* library,
                                                    sta::NetworkReader* network)
{
  // build cell
  Cell* cell = network->makeCell(library, name, false, nullptr);

  // add global ports
  auto pin_iter = db_network_->pinIterator(db_network_->topInstance());
  while (pin_iter->hasNext()) {
    const Pin* pin = pin_iter->next();

    const char* portname = db_network_->name(pin);
    Port* port = network->makePort(cell, portname);
    network->setDirection(port, db_network_->direction(pin));
  }
  delete pin_iter;

  network->groupBusPorts(cell, [](const char*) { return true; });

  // build instance
  std::string instname = name;
  instname += "_inst";
  Instance* inst = network->makeInstance(cell, instname.c_str(), nullptr);

  CellPortBitIterator* port_iter = network->portBitIterator(cell);
  while (port_iter->hasNext()) {
    Port* port = port_iter->next();
    const char* port_name = network->name(port);
    Net* net = network->makeNet(port_name, inst);
    Pin* pin = network->makePin(inst, port, nullptr);
    network->makeTerm(pin, net);
  }
  delete port_iter;

  return inst;
}

odb::dbBlock* PartitionMgr::getDbBlock() const
{
  odb::dbChip* chip = db_->getChip();
  if (!chip) {
    return nullptr;
  }
  return chip->getBlock();
}

void PartitionMgr::writePartitionVerilog(const char* file_name,
                                         const char* port_prefix,
                                         const char* module_suffix)
{
  dbBlock* block = getDbBlock();
  if (block == nullptr) {
    return;
  }

  logger_->info(PAR, 1, "Writing partition to verilog.");
  // get top module name
  const std::string top_name = db_network_->name(db_network_->topInstance());

  // build partition instance map
  std::map<int, std::set<Instance*, CompareInstancePtr>> instance_map;
  for (dbInst* inst : block->getInsts()) {
    dbIntProperty* prop_id = dbIntProperty::find(inst, "partition_id");
    if (!prop_id) {
      logger_->warn(PAR,
                    15,
                    "Property 'partition_id' not found for inst {}.",
                    inst->getName());
    } else {
      const int partition = prop_id->getValue();
      if (instance_map.find(partition) == instance_map.end()) {
        instance_map.insert(
            {partition, std::set<Instance*, CompareInstancePtr>(db_network_)});
      }
      instance_map[partition].insert(db_network_->dbToSta(inst));
    }
  }

  // create new network and library
  NetworkReader* network = sta::makeConcreteNetwork();
  Library* library = network->makeLibrary("Partitions", nullptr);

  // new top module
  Instance* top_inst
      = buildPartitionedTopInstance(top_name.c_str(), library, network);

  // build submodule partitions
  std::map<int, Instance*> sta_instance_map;
  std::map<int, std::map<Net*, Port*>> sta_port_map;
  for (auto& [partition, instances] : instance_map) {
    const std::string cell_name
        = top_name + module_suffix + std::to_string(partition);
    sta_instance_map[partition]
        = buildPartitionedInstance(cell_name.c_str(),
                                   port_prefix,
                                   library,
                                   network,
                                   top_inst,
                                   &instances,
                                   &sta_port_map[partition]);
  }

  // connect submodule partitions in new top module
  for (auto& [partition, instance] : sta_instance_map) {
    for (auto& [portnet, port] : sta_port_map[partition]) {
      const char* net_name = network->name(port);

      Net* net = network->findNet(top_inst, net_name);
      if (net == nullptr) {
        net = network->makeNet(net_name, top_inst);
      }

      network->connect(instance, port, net);
    }
  }

  reinterpret_cast<ConcreteNetwork*>(network)->setTopInstance(top_inst);

  writeVerilog(file_name, true, false, {}, network);

  delete network;
}

// Read partitioning input file

void PartitionMgr::readPartitioningFile(const std::string& filename,
                                        const std::string& instance_map_file)
{
  auto block = getDbBlock();

  // determine order the instances will be in the input file
  std::vector<odb::dbInst*> instance_order;
  if (!instance_map_file.empty()) {
    std::ifstream map_file(instance_map_file);
    if (!map_file) {
      logger_->error(PAR, 25, "Unable to open file {}.", instance_map_file);
    }
    std::string line;
    while (getline(map_file, line)) {
      if (line.empty()) {
        continue;
      }
      auto inst = block->findInst(line.c_str());
      if (!inst) {
        logger_->error(PAR, 26, "Unable to find instance {}.", line);
      }
      instance_order.push_back(inst);
    }
  } else {
    auto insts = block->getInsts();
    instance_order.assign(insts.begin(), insts.end());
  }

  std::vector<int> inst_partitions;
  {
    std::ifstream file(filename);
    if (!file) {
      logger_->error(PAR, 36, "Unable to open file {}.", filename);
    }
    std::string line;
    while (getline(file, line)) {
      if (line.empty()) {
        continue;
      }
      try {
        inst_partitions.push_back(std::stoi(line));
      } catch (const std::logic_error&) {
        logger_->error(
            PAR,
            27,
            "Unable to convert line \"{}\" to an integer in file: {}",
            line,
            filename);
      }
    }
  }

  if (inst_partitions.size() != instance_order.size()) {
    logger_->error(PAR,
                   28,
                   "Instances in partitioning ({}) does not match instances in "
                   "netlist ({}).",
                   inst_partitions.size(),
                   instance_order.size());
  }

  for (size_t i = 0; i < inst_partitions.size(); i++) {
    const auto inst = instance_order[i];
    const auto partition_id = inst_partitions[i];

    if (auto property = odb::dbIntProperty::find(inst, "partition_id")) {
      property->setValue(partition_id);
    } else {
      odb::dbIntProperty::create(inst, "partition_id", partition_id);
    }
  }
}

}  // namespace par

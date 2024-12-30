///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (c) 2021, The Regents of the University of California
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
// ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
///////////////////////////////////////////////////////////////////////////////

#include <chrono>
#include <fstream>
#include <iostream>
#include "leidenClustering.h"
#include <thread>
#include <libleidenalg/GraphHelper.h>
#include <libleidenalg/Optimiser.h>
#include <libleidenalg/ModularityVertexPartition.h>
#include "db_sta/dbNetwork.hh"
#include "par/PartitionMgr.h"
#include "sta/Liberty.hh"
#include "ord/OpenRoad.hh"

namespace odb {
class dbBlock;
class dbMaster;
}  // namespace odb

namespace sta {
class dbNetwork;
}  // namespace sta

namespace utl {
class Logger;
}  // namespace utl

namespace mpl2 {
/**
 * @brief Constructor for the Leiden Clustering class.
 */
leidenClustering::leidenClustering(sta::dbNetwork* network,
                                   odb::dbBlock* block,
                                   utl::Logger* logger)
    : network_(network), block_(block), logger_(logger)
{
}
bool leidenClustering::get_vertex_id()
{
  // Create vertex_id for bterms
  // extend vertex_names_ to length id if exceeds the current length
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    int id = -1;
    odb::dbIntProperty* property = odb::dbIntProperty::find(bterm, "vertex_id");
    if(property) {
      id = property->getValue();
    }
    else{
      return false;
    }
    vertex_names_[id] = bterm->getConstName();
  }
  // Create vertex_id for instances that are not pads, covers or end caps
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (isIgnoredMaster(master) || master->isBlock()) {
      continue;
    }
    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (!liberty_cell) {
      continue;
    }
    int id = -1;
    odb::dbIntProperty* property = odb::dbIntProperty::find(inst, "vertex_id");
    if(property) {
      id = property->getValue();
    }
    else{
      return false;
    }
    vertex_names_[id] = inst->getConstName();
  }
  // Create vertex_id for macro pins that are not signal pins
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (!master->isBlock()) {
      continue;
    }
    for (odb::dbITerm* pin : inst->getITerms()) {
      if (pin->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      int id = -1;
      odb::dbIntProperty* property = odb::dbIntProperty::find(pin, "vertex_id");
      if(property) {
        id = property->getValue();
      }
      else{
        return false;
      }
      vertex_names_[id] = pin->getName();
    }
  }
  logger_->report("Get vertex_id for vertices, Number of vertices: {}", vertex_names_.size());
  return true;
}
void leidenClustering::create_vertex_id() 
{
  int max_id = -1;
  // Create vertex_id for bterms
  // extend vertex_names_ to length id if exceeds the current length
  for (odb::dbBTerm* bterm : block_->getBTerms()) {
    odb::dbIntProperty* property = odb::dbIntProperty::find(bterm, "vertex_id");
    if(property) {
      property->setValue(++max_id);
    }
    else{
      odb::dbIntProperty::create(bterm, "vertex_id", ++max_id);
    }
    vertex_names_[max_id] = bterm->getConstName();
  }
  // Create vertex_id for instances that are not pads, covers or end caps
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (isIgnoredMaster(master) || master->isBlock()) {
      continue;
    }
    odb::dbIntProperty* property = odb::dbIntProperty::find(inst, "vertex_id");
    if(property) {
      property->setValue(++max_id);
    }
    else{
      odb::dbIntProperty::create(inst, "vertex_id", ++max_id);
    }
    vertex_names_[max_id] = inst->getConstName();
  }
  // Create vertex_id for macro pins that are not signal pins
  for (odb::dbInst* inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (!master->isBlock()) {
      continue;
    }
    for (odb::dbITerm* pin : inst->getITerms()) {
      if (pin->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      odb::dbIntProperty* property = odb::dbIntProperty::find(pin, "vertex_id");
      if(property) {
        property->setValue(++max_id);
      }
      else{
        odb::dbIntProperty::create(pin, "vertex_id", ++max_id);
      }
      vertex_names_[max_id] = pin->getName();
    }
  }
}
/**
 * @brief Initializes the igraph for Leiden Clustering algorithm.
 *
 * This function is a placeholder for any initialization steps required
 * before running the clustering algorithm.
 */
void leidenClustering::init(bool create_new_vertex_id)
{
  graph_ = new igraph_t();
  logger_->report("\tRunning Leiden Clustering, initializing...");
  if (create_new_vertex_id) {
    create_vertex_id();
  }
  else {
    if(!get_vertex_id()){
      logger_->report("\t[Warning]: Get vertex id wrong.");
    }
  }
  // Create the igraph object
  int v_count = vertex_names_.size();
  igraph_vector_int_t edges;
  igraph_vector_int_init(&edges, 0);
  for (odb::dbNet* net : block_->getNets()) {
    int source = -1;
    std::set<int> sinks;
    size_t fan_out = 0;
    if (get_net_info(net, source, sinks, fan_out)) {
      for (int sink : sinks) {
        igraph_vector_int_push_back(&edges, source);
        igraph_vector_int_push_back(&edges, sink);
        edge_weights_.push_back(fan_out);
      }
    }
  }
  igraph_create(graph_, &edges, v_count, true);
  igraph_vector_int_destroy(&edges);
}

/**
 * @brief Runs the Leiden Clustering algorithm.
 *
 * This function initializes the clustering process, creates the hypergraph,
 * and then creates the graph from the hypergraph.
 */
void leidenClustering::run()
{
  logger_->report("Running Leiden Clustering...");
  runLeidenClustering();
  igraph_destroy(graph_);
}

bool leidenClustering::isIgnoredMaster(odb::dbMaster* master)
{
  // IO corners are sometimes marked as end caps
  return master->isPad() || master->isCover() || master->isEndCap();
}

void leidenClustering::runLeidenClustering()
{
  // Initialize the partition using the Modularity partition strategy
  logger_->report("\tStart Creating initial partition.");
  LeidenGraphInterface* graph = LeidenGraphInterface::GraphFromEdgeWeights(graph_, edge_weights_);
  auto start = std::chrono::high_resolution_clock::now();
  ModularityVertexPartition part(graph);
  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> duration = end - start;
  logger_->report("\tTime taken to create partition: {} seconds",
                  duration.count());

  // Initialize the optimiser
  Optimiser optimiser;
  logger_->report("\tStart Running Leiden Clustering");

  // Perform optimization on the partition
  start = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < iteration_; i++) {
    optimiser.optimise_partition(&part);
  }
  end = std::chrono::high_resolution_clock::now();
  duration = end - start;

  logger_->report("\tTime taken to optimize partition: {} seconds",
                  duration.count());
  partition_.reserve(vertex_names_.size());
  size_t max_cluster_id = 0;
  for (int i = 0; i < vertex_names_.size(); i++) {
    max_cluster_id = std::max(max_cluster_id, part.membership(i));
    partition_.push_back(part.membership(i));
  }
  logger_->report("\tFinished running Leiden Clustering, max cluster id: {}", max_cluster_id);
  max_cluster_id_ = max_cluster_id;
}

void leidenClustering::write_partition_csv(const std::string& file_name)
{
  logger_->report("\tRunning writing partition, initializing...");
  std::vector<std::vector<std::string>> groups(max_cluster_id_ + 1);
  for (size_t i = 0; i < partition_.size(); i++) {
    groups[partition_[i]].push_back(vertex_names_[i]);
  }
  std::ofstream file(file_name);
  for (size_t i = 0; i < groups.size(); i++) {
    file << "group: " << i << std::endl;
    for (size_t j = 0; j < groups[i].size(); j++) {
      file << " - " << groups[i][j] << std::endl;
    }
  }
  logger_->report("Partition data written to {}", file_name);
}

void leidenClustering::write_graph_csv(const std::string& edge_file,
                                       const std::string& node_file)
{
  logger_->report("\tRunning writing graph, initializing...");

    auto get_net_fanout = [](odb::dbNet* net) {
    size_t fanout = 0;
    for (odb::dbITerm* iterm : net->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        fanout++;
      }
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
        fanout++;
      }
    }
    return fanout;
  };

  auto get_net_source = [](odb::dbNet* net) {
    for (odb::dbITerm* iterm : net->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        return iterm->getInst()->getName();
      }
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        return bterm->getName();
      }
    }
    return std::string();
  };

  auto get_net_sinks = [](odb::dbNet* net) {
    std::vector<std::string> sinks;
    for (odb::dbITerm* iterm : net->getITerms()) {
      if (iterm->getIoType() == odb::dbIoType::INPUT) {
        sinks.push_back(iterm->getInst()->getName());
      }
    }
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      if (bterm->getIoType() == odb::dbIoType::OUTPUT) {
        sinks.push_back(bterm->getName());
      }
    }
    return sinks;
  };

  auto print_node_header
      = [](std::ofstream& out) { out << "Name,Type,Master,Height,Width\n"; };

  auto print_node = [](std::ofstream& out, odb::dbInst* inst) {
    odb::dbMaster* master = inst->getMaster();
    out << inst->getName() << "," << "inst"
        << "," << master->getName() << "," << master->getHeight() << ","
        << master->getWidth() << "\n";
  };

  auto print_edge_header
      = [](std::ofstream& out) { out << "Source,Sink,Weight,Net\n"; };

  auto print_edge = [&](std::ofstream& out, odb::dbNet* net) {
    size_t net_fanout = get_net_fanout(net);
    if (net->getSigType() == odb::dbSigType::POWER || net->getSigType() == odb::dbSigType::GROUND || net_fanout > 50 || net_fanout == 0) {
      return;
    }

    float edge_weight = 1.0 / net_fanout;
    std::string source_name = get_net_source(net);
    std::vector<std::string> sinks = get_net_sinks(net);

    if (sinks.size() != net_fanout) {
      logger_->report("Fanout list of {} is not unique.", net->getConstName());
    }

    for (const std::string& sink : sinks) {
      out << source_name << "," << sink << "," << edge_weight << ","
          << net->getConstName() << "\n";
    }
  };

  std::ofstream edge_out(edge_file);
  std::ofstream node_out(node_file);

  if (!edge_out.is_open() || !node_out.is_open()) {
    logger_->report("Failed to open output files for writing graph data.");
    return;
  }

  print_node_header(node_out);
  print_edge_header(edge_out);

  for (auto inst : block_->getInsts()) {
    print_node(node_out, inst);
  }

  for (auto bterm : block_->getBTerms()) {
    node_out << bterm->getName() << ",term,NA,0.0,0.0\n";
  }

  for (auto net : block_->getNets()) {
    print_edge(edge_out, net);
  }

  edge_out.close();
  node_out.close();
  logger_->report("Graph data written to {} and {}", edge_file, node_file);
}

void leidenClustering::write_placement_csv(const std::string& file_name)
{
  logger_->report("\tRunning writing placement, initializing...");

  auto print_header = [](std::ofstream& out) { out << "Name,Type,Master,X,Y\n"; };
  auto write_placement_info_helper = [](std::ofstream& out, odb::dbInst* inst) {
    odb::dbPlacementStatus status = inst->getPlacementStatus();
    if (status == odb::dbPlacementStatus::NONE) {
      return;
    }
    odb::Point pos = inst->getOrigin();
    odb::dbMaster* master = inst->getMaster();
    out << inst->getName() << ",inst," << master->getName() << "," << pos.x() << "," << pos.y() << "\n";
  };

  std::ofstream out(file_name);
  if (!out.is_open()) {
    logger_->report("Failed to open output file for writing placement data.");
    return;
  }

  print_header(out);
  for (auto inst : block_->getInsts()) {
    write_placement_info_helper(out, inst);
  }

  for (auto term : block_->getBTerms()) {
    int x, y;
    term->getFirstPinLocation(x, y);
    out << term->getName() << ",term,NA," << x << "," << y << "\n";
  }

  out.close();
  logger_->report("Placement data written to {}", file_name);
}

}  // namespace mpl2
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

#include "clusterEngine.h"

#include "db_sta/dbNetwork.hh"
#include "object.h"
#include "sta/Liberty.hh"

namespace mpl2 {
using utl::MPL;

ClusteringEngine::ClusteringEngine(odb::dbBlock* block,
                                   sta::dbNetwork* network,
                                   utl::Logger* logger)
    : block_(block), network_(network), logger_(logger), id_(0)
{
}

void ClusteringEngine::buildPhysicalHierarchy()
{
  initTree();
  setDefaultThresholds();

  createIOClusters();
  createDataFlow();
}

void ClusteringEngine::setDesignMetrics(Metrics* design_metrics)
{
  design_metrics_ = design_metrics;
}

void ClusteringEngine::setTargetStructure(PhysicalHierarchy* tree)
{
  tree_ = tree;
}

void ClusteringEngine::initTree()
{
  setDefaultThresholds();

  tree_->root = new Cluster(id_, std::string("root"), logger_);
  tree_->root->addDbModule(block_->getTopModule());
  tree_->root->setMetrics(*design_metrics_);

  tree_->maps.id_to_cluster[id_++] = tree_->root;

  // Associate all instances to root
  for (odb::dbInst* inst : block_->getInsts()) {
    tree_->maps.inst_to_cluster_id[inst] = id_;
  }
}

void ClusteringEngine::setDefaultThresholds()
{
  if (tree_->base_thresholds.max_macro <= 0
      || tree_->base_thresholds.min_macro <= 0
      || tree_->base_thresholds.max_std_cell <= 0
      || tree_->base_thresholds.min_std_cell <= 0) {
    // Set base values for std cell lower/upper thresholds
    const int min_num_std_cells_allowed = 1000;
    tree_->base_thresholds.min_std_cell
        = std::floor(design_metrics_->getNumStdCell()
                     / std::pow(tree_->coarsening_ratio, tree_->max_level));
    if (tree_->base_thresholds.min_std_cell <= min_num_std_cells_allowed) {
      tree_->base_thresholds.min_std_cell = min_num_std_cells_allowed;
    }
    tree_->base_thresholds.max_std_cell
        = tree_->base_thresholds.min_std_cell * tree_->coarsening_ratio / 2.0;

    // Set base values for macros lower/upper thresholds
    tree_->base_thresholds.min_macro
        = std::floor(design_metrics_->getNumMacro()
                     / std::pow(tree_->coarsening_ratio, tree_->max_level));
    if (tree_->base_thresholds.min_macro <= 0) {
      tree_->base_thresholds.min_macro = 1;
    }
    tree_->base_thresholds.max_macro
        = tree_->base_thresholds.min_macro * tree_->coarsening_ratio / 2.0;

    // From original implementation: Reset maximum level based on number
    // of macros.
    const int min_num_macros_for_multilevel = 150;
    if (design_metrics_->getNumMacro() <= min_num_macros_for_multilevel) {
      tree_->max_level = 1;
      debugPrint(
          logger_,
          MPL,
          "multilevel_autoclustering",
          1,
          "Number of macros is below {}. Resetting number of levels to {}",
          min_num_macros_for_multilevel,
          tree_->max_level);
    }
  }

  // Set sizes for root
  unsigned coarsening_factor
      = std::pow(tree_->coarsening_ratio, tree_->max_level - 1);
  tree_->base_thresholds.max_macro *= coarsening_factor;
  tree_->base_thresholds.min_macro *= coarsening_factor;
  tree_->base_thresholds.max_std_cell *= coarsening_factor;
  tree_->base_thresholds.min_std_cell *= coarsening_factor;

  debugPrint(
      logger_,
      MPL,
      "multilevel_autoclustering",
      1,
      "num level: {}, max_macro: {}, min_macro: {}, max_inst:{}, min_inst:{}",
      tree_->max_level,
      tree_->base_thresholds.max_macro,
      tree_->base_thresholds.min_macro,
      tree_->base_thresholds.max_std_cell,
      tree_->base_thresholds.min_std_cell);
}

void ClusteringEngine::createIOClusters()
{
  mapIOPads();

  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Creating bundledIO clusters...");

  const odb::Rect die = block_->getDieArea();

  // Get the floorplan information and get the range of bundled IO regions
  odb::Rect die_box = block_->getCoreArea();
  int core_lx = die_box.xMin();
  int core_ly = die_box.yMin();
  int core_ux = die_box.xMax();
  int core_uy = die_box.yMax();
  const int x_base = (die.xMax() - die.xMin()) / tree_->bundled_ios_per_edge;
  const int y_base = (die.yMax() - die.yMin()) / tree_->bundled_ios_per_edge;
  int cluster_id_base = id_;

  // Map all the BTerms / Pads to Bundled IOs (cluster)
  std::vector<std::string> prefix_vec;
  prefix_vec.emplace_back("L");
  prefix_vec.emplace_back("T");
  prefix_vec.emplace_back("R");
  prefix_vec.emplace_back("B");
  std::map<int, bool> cluster_io_map;
  for (int i = 0; i < 4;
       i++) {  // four boundaries (Left, Top, Right and Bottom in order)
    for (int j = 0; j < tree_->bundled_ios_per_edge; j++) {
      std::string cluster_name = prefix_vec[i] + std::to_string(j);
      Cluster* cluster = new Cluster(id_, cluster_name, logger_);
      tree_->root->addChild(cluster);
      cluster->setParent(tree_->root);
      cluster_io_map[id_] = false;
      tree_->maps.id_to_cluster[id_++] = cluster;
      int x = 0.0;
      int y = 0.0;
      int width = 0;
      int height = 0;
      if (i == 0) {  // Left boundary
        x = die.xMin();
        y = die.yMin() + y_base * j;
        height = y_base;
      } else if (i == 1) {  // Top boundary
        x = die.xMin() + x_base * j;
        y = die.yMax();
        width = x_base;
      } else if (i == 2) {  // Right boundary
        x = die.xMax();
        y = die.yMax() - y_base * (j + 1);
        height = y_base;
      } else {  // Bottom boundary
        x = die.xMax() - x_base * (j + 1);
        y = die.yMin();
        width = x_base;
      }

      // set the cluster to a IO cluster
      cluster->setAsIOCluster(std::pair<float, float>(block_->dbuToMicrons(x),
                                                      block_->dbuToMicrons(y)),
                              block_->dbuToMicrons(width),
                              block_->dbuToMicrons(height));
    }
  }

  // Map all the BTerms to bundled IOs
  for (auto term : block_->getBTerms()) {
    int lx = std::numeric_limits<int>::max();
    int ly = std::numeric_limits<int>::max();
    int ux = 0;
    int uy = 0;
    // If the design has IO pads, these block terms
    // will not have block pins.
    // Otherwise, the design will have IO pins.
    for (const auto pin : term->getBPins()) {
      for (const auto box : pin->getBoxes()) {
        lx = std::min(lx, box->xMin());
        ly = std::min(ly, box->yMin());
        ux = std::max(ux, box->xMax());
        uy = std::max(uy, box->yMax());
      }
    }
    // remove power pins
    if (term->getSigType().isSupply()) {
      continue;
    }

    // If the term has a connected pad, get the bbox from the pad inst
    if (tree_->maps.bterm_to_inst.find(term)
        != tree_->maps.bterm_to_inst.end()) {
      lx = tree_->maps.bterm_to_inst[term]->getBBox()->xMin();
      ly = tree_->maps.bterm_to_inst[term]->getBBox()->yMin();
      ux = tree_->maps.bterm_to_inst[term]->getBBox()->xMax();
      uy = tree_->maps.bterm_to_inst[term]->getBBox()->yMax();
      if (lx <= core_lx) {
        lx = die.xMin();
      }
      if (ly <= core_ly) {
        ly = die.yMin();
      }
      if (ux >= core_ux) {
        ux = die.xMax();
      }
      if (uy >= core_uy) {
        uy = die.yMax();
      }
    }
    // calculate cluster id based on the location of IO Pins / Pads
    int cluster_id = -1;
    if (lx <= die.xMin()) {
      // The IO is on the left boundary
      cluster_id = cluster_id_base
                   + std::floor(((ly + uy) / 2.0 - die.yMin()) / y_base);
    } else if (uy >= die.yMax()) {
      // The IO is on the top boundary
      cluster_id = cluster_id_base + tree_->bundled_ios_per_edge
                   + std::floor(((lx + ux) / 2.0 - die.xMin()) / x_base);
    } else if (ux >= die.xMax()) {
      // The IO is on the right boundary
      cluster_id = cluster_id_base + tree_->bundled_ios_per_edge * 2
                   + std::floor((die.yMax() - (ly + uy) / 2.0) / y_base);
    } else if (ly <= die.yMin()) {
      // The IO is on the bottom boundary
      cluster_id = cluster_id_base + tree_->bundled_ios_per_edge * 3
                   + std::floor((die.xMax() - (lx + ux) / 2.0) / x_base);
    }

    // Check if the IO pins / Pads exist
    if (cluster_id == -1) {
      logger_->error(
          MPL,
          2,
          "Floorplan has not been initialized? Pin location error for {}.",
          term->getName());
    } else {
      tree_->maps.bterm_to_cluster_id[term] = cluster_id;
    }

    cluster_io_map[cluster_id] = true;
  }

  // delete the IO clusters that do not have any pins assigned to them
  for (auto& [cluster_id, flag] : cluster_io_map) {
    if (!flag) {
      debugPrint(logger_,
                 MPL,
                 "multilevel_autoclustering",
                 1,
                 "Remove IO Cluster with no pins: {}, id: {}",
                 tree_->maps.id_to_cluster[cluster_id]->getName(),
                 cluster_id);
      tree_->maps.id_to_cluster[cluster_id]->getParent()->removeChild(
          tree_->maps.id_to_cluster[cluster_id]);
      delete tree_->maps.id_to_cluster[cluster_id];
      tree_->maps.id_to_cluster.erase(cluster_id);
    }
  }

  // At this point the cluster map has only the root (id = 0) and bundledIOs
  if (tree_->maps.id_to_cluster.size() == 1) {
    logger_->warn(MPL, 26, "Design has no IO pins!");
    tree_->has_io_clusters = false;
  }
}

void ClusteringEngine::mapIOPads()
{
  bool design_has_io_pads = false;
  for (auto inst : block_->getInsts()) {
    if (inst->getMaster()->isPad()) {
      design_has_io_pads = true;
      break;
    }
  }

  if (!design_has_io_pads) {
    return;
  }

  for (odb::dbNet* net : block_->getNets()) {
    if (net->getBTerms().size() == 0) {
      continue;
    }

    // If the design has IO pads, there is a net
    // connecting the IO pin and IO pad instance
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      for (odb::dbITerm* iterm : net->getITerms()) {
        odb::dbInst* inst = iterm->getInst();
        tree_->maps.bterm_to_inst[bterm] = inst;
      }
    }
  }
}

// Dataflow is used to improve quality of macro placement.
// Here we model each std cell instance, IO pin and macro pin as vertices.
void ClusteringEngine::createDataFlow()
{
  debugPrint(
      logger_, MPL, "multilevel_autoclustering", 1, "Creating dataflow...");
  if (data_flow.register_dist <= 0) {
    return;
  }

  // create vertex id property for std cell, IO pin and macro pin
  std::map<int, odb::dbBTerm*> io_pin_vertex;
  std::map<int, odb::dbInst*> std_cell_vertex;
  std::map<int, odb::dbITerm*> macro_pin_vertex;

  std::vector<bool> stop_flag_vec;
  // assign vertex_id property of each Bterm
  // All boundary terms are marked as sequential stopping pts
  for (odb::dbBTerm* term : block_->getBTerms()) {
    odb::dbIntProperty::create(term, "vertex_id", stop_flag_vec.size());
    io_pin_vertex[stop_flag_vec.size()] = term;
    stop_flag_vec.push_back(true);
  }

  // assign vertex_id property of each instance
  for (auto inst : block_->getInsts()) {
    odb::dbMaster* master = inst->getMaster();
    if (isIgnoredMaster(master) || master->isBlock()) {
      continue;
    }

    const sta::LibertyCell* liberty_cell = network_->libertyCell(inst);
    if (!liberty_cell) {
      continue;
    }

    // Mark registers
    odb::dbIntProperty::create(inst, "vertex_id", stop_flag_vec.size());
    std_cell_vertex[stop_flag_vec.size()] = inst;

    if (liberty_cell->hasSequentials()) {
      stop_flag_vec.push_back(true);
    } else {
      stop_flag_vec.push_back(false);
    }
  }
  // assign vertex_id property of each macro pin
  // all macro pins are flagged as sequential stopping pt
  for (auto& [macro, hard_macro] : tree_->maps.inst_to_hard) {
    for (odb::dbITerm* pin : macro->getITerms()) {
      if (pin->getSigType() != odb::dbSigType::SIGNAL) {
        continue;
      }
      odb::dbIntProperty::create(pin, "vertex_id", stop_flag_vec.size());
      macro_pin_vertex[stop_flag_vec.size()] = pin;
      stop_flag_vec.push_back(true);
    }
  }

  //
  // Num of vertices will be # of boundary pins + number of logical std cells +
  // number of macro pins)
  //
  debugPrint(logger_,
             MPL,
             "multilevel_autoclustering",
             1,
             "Number of vertices: {}",
             stop_flag_vec.size());

  // create hypergraphs
  std::vector<std::vector<int>> vertices(stop_flag_vec.size());
  std::vector<std::vector<int>> backward_vertices(stop_flag_vec.size());
  std::vector<std::vector<int>> hyperedges;  // dircted hypergraph
  // traverse the netlist
  for (odb::dbNet* net : block_->getNets()) {
    // ignore all the power net
    if (net->getSigType().isSupply()) {
      continue;
    }
    int driver_id = -1;      // driver vertex id
    std::set<int> loads_id;  // load vertex id
    bool ignore = false;
    // check the connected instances
    for (odb::dbITerm* iterm : net->getITerms()) {
      odb::dbInst* inst = iterm->getInst();
      odb::dbMaster* master = inst->getMaster();
      // We ignore nets connecting ignored masters
      if (isIgnoredMaster(master)) {
        ignore = true;
        break;
      }
      int vertex_id = -1;
      if (master->isBlock()) {
        vertex_id = odb::dbIntProperty::find(iterm, "vertex_id")->getValue();
      } else {
        vertex_id = odb::dbIntProperty::find(inst, "vertex_id")->getValue();
      }
      if (iterm->getIoType() == odb::dbIoType::OUTPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }
    if (ignore) {
      continue;  // the nets with Pads should be ignored
    }

    // check the connected IO pins  of the net
    for (odb::dbBTerm* bterm : net->getBTerms()) {
      const int vertex_id
          = odb::dbIntProperty::find(bterm, "vertex_id")->getValue();
      if (bterm->getIoType() == odb::dbIoType::INPUT) {
        driver_id = vertex_id;
      } else {
        loads_id.insert(vertex_id);
      }
    }

    //
    // Skip high fanout nets or nets that do not have valid driver or loads
    //
    if (driver_id < 0 || loads_id.empty()
        || loads_id.size() > tree_->large_net_threshold) {
      continue;
    }

    // Create the hyperedge
    std::vector<int> hyperedge{driver_id};
    for (auto& load : loads_id) {
      if (load != driver_id) {
        hyperedge.push_back(load);
      }
    }
    vertices[driver_id].push_back(hyperedges.size());
    for (int i = 1; i < hyperedge.size(); i++) {
      backward_vertices[hyperedge[i]].push_back(hyperedges.size());
    }
    hyperedges.push_back(hyperedge);
  }  // end net traversal

  debugPrint(
      logger_, MPL, "multilevel_autoclustering", 1, "Created hypergraph");

  // traverse hypergraph to build dataflow
  for (auto [src, src_pin] : io_pin_vertex) {
    int idx = 0;
    std::vector<bool> visited(vertices.size(), false);
    std::vector<std::set<odb::dbInst*>> insts(data_flow.register_dist);
    dataFlowDFSIOPin(src,
                     idx,
                     insts,
                     io_pin_vertex,
                     std_cell_vertex,
                     macro_pin_vertex,
                     stop_flag_vec,
                     visited,
                     vertices,
                     hyperedges,
                     false);
    dataFlowDFSIOPin(src,
                     idx,
                     insts,
                     io_pin_vertex,
                     std_cell_vertex,
                     macro_pin_vertex,
                     stop_flag_vec,
                     visited,
                     backward_vertices,
                     hyperedges,
                     true);
    data_flow.io_to_regs.emplace_back(src_pin, insts);
  }

  for (auto [src, src_pin] : macro_pin_vertex) {
    int idx = 0;
    std::vector<bool> visited(vertices.size(), false);
    std::vector<std::set<odb::dbInst*>> std_cells(data_flow.register_dist);
    std::vector<std::set<odb::dbInst*>> macros(data_flow.register_dist);
    dataFlowDFSMacroPin(src,
                        idx,
                        std_cells,
                        macros,
                        io_pin_vertex,
                        std_cell_vertex,
                        macro_pin_vertex,
                        stop_flag_vec,
                        visited,
                        vertices,
                        hyperedges,
                        false);
    dataFlowDFSMacroPin(src,
                        idx,
                        std_cells,
                        macros,
                        io_pin_vertex,
                        std_cell_vertex,
                        macro_pin_vertex,
                        stop_flag_vec,
                        visited,
                        backward_vertices,
                        hyperedges,
                        true);
    data_flow.macro_pin_to_regs.emplace_back(src_pin, std_cells);
    data_flow.macro_pin_to_macros.emplace_back(src_pin, macros);
  }
}

/* static */
bool ClusteringEngine::isIgnoredMaster(odb::dbMaster* master)
{
  // IO corners are sometimes marked as end caps
  return master->isPad() || master->isCover() || master->isEndCap();
}

// Forward or Backward DFS search to find sequential paths from/to IO pins based
// on hop count to macro pins
void ClusteringEngine::dataFlowDFSIOPin(
    int parent,
    int idx,
    std::vector<std::set<odb::dbInst*>>& insts,
    std::map<int, odb::dbBTerm*>& io_pin_vertex,
    std::map<int, odb::dbInst*>& std_cell_vertex,
    std::map<int, odb::dbITerm*>& macro_pin_vertex,
    std::vector<bool>& stop_flag_vec,
    std::vector<bool>& visited,
    std::vector<std::vector<int>>& vertices,
    std::vector<std::vector<int>>& hyperedges,
    bool backward_search)
{
  visited[parent] = true;
  if (stop_flag_vec[parent]) {
    if (parent < io_pin_vertex.size()) {
      ;  // currently we do not consider IO pin to IO pin connection
    } else if (parent < io_pin_vertex.size() + std_cell_vertex.size()) {
      insts[idx].insert(std_cell_vertex[parent]);
    } else {
      insts[idx].insert(macro_pin_vertex[parent]->getInst());
    }
    idx++;
  }

  if (idx >= data_flow.register_dist) {
    return;
  }

  if (!backward_search) {
    for (auto& hyperedge : vertices[parent]) {
      for (auto& vertex : hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] || vertex < io_pin_vertex.size()) {
          continue;
        }
        dataFlowDFSIOPin(vertex,
                         idx,
                         insts,
                         io_pin_vertex,
                         std_cell_vertex,
                         macro_pin_vertex,
                         stop_flag_vec,
                         visited,
                         vertices,
                         hyperedges,
                         backward_search);
      }
    }
  } else {
    for (auto& hyperedge : vertices[parent]) {
      const int vertex = hyperedges[hyperedge][0];  // driver vertex
      // we do not consider pin to pin
      if (visited[vertex] || vertex < io_pin_vertex.size()) {
        continue;
      }
      dataFlowDFSIOPin(vertex,
                       idx,
                       insts,
                       io_pin_vertex,
                       std_cell_vertex,
                       macro_pin_vertex,
                       stop_flag_vec,
                       visited,
                       vertices,
                       hyperedges,
                       backward_search);
    }
  }
}

// Forward or Backward DFS search to find sequential paths between Macros based
// on hop count
void ClusteringEngine::dataFlowDFSMacroPin(
    int parent,
    int idx,
    std::vector<std::set<odb::dbInst*>>& std_cells,
    std::vector<std::set<odb::dbInst*>>& macros,
    std::map<int, odb::dbBTerm*>& io_pin_vertex,
    std::map<int, odb::dbInst*>& std_cell_vertex,
    std::map<int, odb::dbITerm*>& macro_pin_vertex,
    std::vector<bool>& stop_flag_vec,
    std::vector<bool>& visited,
    std::vector<std::vector<int>>& vertices,
    std::vector<std::vector<int>>& hyperedges,
    bool backward_search)
{
  visited[parent] = true;
  if (stop_flag_vec[parent]) {
    if (parent < io_pin_vertex.size()) {
      ;  // the connection between IO and macro pins have been considers
    } else if (parent < io_pin_vertex.size() + std_cell_vertex.size()) {
      std_cells[idx].insert(std_cell_vertex[parent]);
    } else {
      macros[idx].insert(macro_pin_vertex[parent]->getInst());
    }
    idx++;
  }

  if (idx >= data_flow.register_dist) {
    return;
  }

  if (!backward_search) {
    for (auto& hyperedge : vertices[parent]) {
      for (auto& vertex : hyperedges[hyperedge]) {
        // we do not consider pin to pin
        if (visited[vertex] || vertex < io_pin_vertex.size()) {
          continue;
        }
        dataFlowDFSMacroPin(vertex,
                            idx,
                            std_cells,
                            macros,
                            io_pin_vertex,
                            std_cell_vertex,
                            macro_pin_vertex,
                            stop_flag_vec,
                            visited,
                            vertices,
                            hyperedges,
                            backward_search);
      }
    }
  } else {
    for (auto& hyperedge : vertices[parent]) {
      const int vertex = hyperedges[hyperedge][0];
      // we do not consider pin to pin
      if (visited[vertex] || vertex < io_pin_vertex.size()) {
        continue;
      }
      dataFlowDFSMacroPin(vertex,
                          idx,
                          std_cells,
                          macros,
                          io_pin_vertex,
                          std_cell_vertex,
                          macro_pin_vertex,
                          stop_flag_vec,
                          visited,
                          vertices,
                          hyperedges,
                          backward_search);
    }
  }
}

}  // namespace mpl2
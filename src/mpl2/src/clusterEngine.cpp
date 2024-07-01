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

#include "object.h"

namespace mpl2 {
using utl::MPL;

ClusteringEngine::ClusteringEngine(odb::dbBlock* block, utl::Logger* logger)
    : block_(block), logger_(logger), id_(0)
{
}

void ClusteringEngine::buildPhysicalHierarchy()
{
  initTree();
  setDefaultThresholds();

  createIOClusters();
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

}  // namespace mpl2
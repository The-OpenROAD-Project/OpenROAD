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

}  // namespace mpl2
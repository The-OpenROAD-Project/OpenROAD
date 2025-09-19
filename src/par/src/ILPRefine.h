// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Refiner.h"
#include "Utilities.h"

namespace par {

class IlpRefine;
using IlpRefinerPtr = std::shared_ptr<IlpRefine>;

// ------------------------------------------------------------------------------
// K-way ILP Based Refinement
// ILP Based Refinement is usually very slow and cannot handle path related
// cost.  Please try to avoid using ILP-Based Refinement for K-way partitioning
// and path-related partitioning.  But ILP Based Refinement is good for 2=way
// min-cut problem
// --------------------------------------------------------------------------------
class IlpRefine : public Refiner
{
 public:
  using Refiner::Refiner;

 private:
  // In each pass, we only move the boundary vertices
  // here we pass block_balance and net_degrees as reference
  // because we only move a few vertices during each pass
  // i.e., block_balance and net_degs will not change too much
  // so we precompute the block_balance and net_degs
  // the return value is the gain improvement
  float Pass(const HGraphPtr& hgraph,
             const Matrix<float>& upper_block_balance,
             const Matrix<float>& lower_block_balance,
             Matrix<float>& block_balance,        // the current block balance
             Matrix<int>& net_degs,               // the current net degree
             std::vector<float>& cur_paths_cost,  // the current path cost
             Partitions& solution,
             std::vector<bool>& visited_vertices_flag) override;
};

}  // namespace par

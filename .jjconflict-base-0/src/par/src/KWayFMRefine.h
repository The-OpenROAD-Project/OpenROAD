// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2022-2025, The OpenROAD Authors

#pragma once

#include <memory>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Refiner.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

class KWayFMRefine;
using KWayFMRefinerPtr = std::shared_ptr<KWayFMRefine>;

// --------------------------------------------------------------------------
// FM-based direct k-way refinement
// --------------------------------------------------------------------------
class KWayFMRefine : public Refiner
{
 public:
  // We have one more parameter related to "corking effect"
  KWayFMRefine(
      int num_parts,
      int refiner_iters,
      float path_wt_factor,     // weight for cutting a critical timing path
      float snaking_wt_factor,  // weight for snaking timing paths
      int max_move,  // the maximum number of vertices or hyperedges can
                     // be moved in each pass
      int total_corking_passes,
      EvaluatorPtr evaluator,
      utl::Logger* logger);

  // Mark these two functions as public.
  // Because they will be called by multi-threading
  // Initialize the single bucket
  void InitializeSingleGainBucket(
      GainBuckets& buckets,
      int to_pid,  // move the vertex into this block (block_id = to_pid)
      const HGraphPtr& hgraph,
      const std::vector<int>& boundary_vertices,
      const Matrix<int>& net_degs,
      const std::vector<float>& cur_paths_cost,
      const Partitions& solution) const;

  // After moving one vertex, the gain of its neighbors will also need
  // to be updated. This function is used to update the gain of neighbor
  // vertices notices that the neighbors has been calculated based on solution,
  // visited status, boundary vertices status
  void UpdateSingleGainBucket(int part,
                              GainBuckets& buckets,
                              const HGraphPtr& hgraph,
                              const std::vector<int>& neighbors,
                              const Matrix<int>& net_degs,
                              const std::vector<float>& cur_paths_cost,
                              const Partitions& solution) const;

 protected:
  // The main function for the FM-based refinement
  // In each pass, we only move the boundary vertices
  float Pass(const HGraphPtr& hgraph,
             const Matrix<float>& upper_block_balance,
             const Matrix<float>& lower_block_balance,
             Matrix<float>& block_balance,        // the current block balance
             Matrix<int>& net_degs,               // the current net degree
             std::vector<float>& cur_paths_cost,  // the current path cost
             Partitions& solution,
             std::vector<bool>& visited_vertices_flag) override;

  // gain bucket related functions
  // Initialize the gain buckets in parallel
  void InitializeGainBucketsKWay(GainBuckets& buckets,
                                 const HGraphPtr& hgraph,
                                 const std::vector<int>& boundary_vertices,
                                 const Matrix<int>& net_degs,
                                 const std::vector<float>& cur_paths_cost,
                                 const Partitions& solution) const;

  // Determine which vertex gain to be picked
  std::shared_ptr<VertexGain> PickMoveKWay(
      GainBuckets& buckets,
      const HGraphPtr& hgraph,
      const Matrix<float>& curr_block_balance,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance) const;

  // move one vertex based on the calculated gain_cell
  void AcceptKWayMove(const std::shared_ptr<VertexGain>& gain_cell,
                      GainBuckets& gain_buckets,
                      std::vector<GainCell>& moves_trace,
                      float& total_delta_gain,
                      std::vector<bool>& visited_vertices_flag,
                      const HGraphPtr& hgraph,
                      Matrix<float>& curr_block_balance,
                      Matrix<int>& net_degs,
                      std::vector<float>& cur_paths_cost,
                      std::vector<int>& solution) const;

  // Remove vertex from a heap
  // Remove the vertex id related vertex gain
  void HeapEleDeletion(int vertex_id, int part, GainBuckets& buckets) const;

  // variables
  int total_corking_passes_ = 25;  // the maximum level of traversing the
                                   // buckets to solve the "corking effect"
};

}  // namespace par

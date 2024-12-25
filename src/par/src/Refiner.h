///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2022, The Regents of the University of California
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
#pragma once

#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "PriorityQueue.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// The algorithms we support
enum class RefinerChoice
{
  GREEDY,         // greedy refinement. try to one entire hyperedge each time
  FLAT_K_WAY_FM,  // direct k-way FM
  KPM_FM,         // K-way pair-wise FM
  ILP_REFINE      // ILP-based partitioning (only for two-way since k-way ILP
                  // partitioning is too timing-consuming)
};

class VertexGain;
using GainCell = std::shared_ptr<VertexGain>;  // for abbreviation

class HyperedgeGain;
using HyperedgeGainPtr = std::shared_ptr<HyperedgeGain>;

// Priority-queue based gain bucket
using GainBucket = std::shared_ptr<PriorityQueue>;
using GainBuckets = std::vector<GainBucket>;

// Hyperedge Gain.
// Compared to VertexGain, there is no source_part_
// Because this hyperedge spans multiple blocks
class HyperedgeGain
{
 public:
  HyperedgeGain(int hyperedge_id,
                int destination_part,
                float gain,
                const std::map<int, float>& path_cost);

  float GetGain() const { return gain_; }
  void SetGain(float gain) { gain_ = gain; }

  int GetHyperedge() const { return hyperedge_id_; }

  int GetDestinationPart() const { return destination_part_; }

  const std::map<int, float>& GetPathCost() const { return path_cost_; }

 private:
  const int hyperedge_id_ = -1;
  const int destination_part_ = -1;  // the destination block id
  float gain_ = 0.0;

  // The updated DELTA path cost after moving vertex the path_cost
  // will change because we will dynamically update the the weight of
  // the path based on the number of the cut on the path
  const std::map<int, float> path_cost_;
};

// ------------------------------------------------------------------------
// The abstract base class for refinement.
// It implements the most basic functions for refinement and provides
// the basic parameters.  Note that the Refiner is an operator class
// It should not modify the hypergraph itself
// ------------------------------------------------------------------------
class Refiner
{
 public:
  Refiner(int num_parts,
          int refiner_iters,
          float path_wt_factor,     // weight for cutting a critical timing path
          float snaking_wt_factor,  // weight for snaking timing paths
          int max_move,  // the maximum number of vertices or hyperedges can
                         // be moved in each pass
          EvaluatorPtr evaluator,
          utl::Logger* logger);

  Refiner(const Refiner&) = delete;
  Refiner(Refiner&) = delete;
  virtual ~Refiner() = default;

  // The main function
  void Refine(const HGraphPtr& hgraph,
              const Matrix<float>& upper_block_balance,
              const Matrix<float>& lower_block_balance,
              Partitions& solution);

  void SetMaxMove(int max_move);
  void SetRefineIters(int refiner_iters);

  void RestoreDefaultParameters();

 protected:
  virtual float Pass(const HGraphPtr& hgraph,
                     const Matrix<float>& upper_block_balance,
                     const Matrix<float>& lower_block_balance,
                     Matrix<float>& block_balance,  // the current block balance
                     Matrix<int>& net_degs,         // the current net degree
                     std::vector<float>& paths_cost,  // the current path cost
                     Partitions& solution,
                     std::vector<bool>& visited_vertices_flag)
      = 0;

  // If to_pid == -1, we are calculate the current cost of the path;
  // else if to_pid != -1, we are calculate the cost of the path
  // after moving v to block to_pid
  float CalculatePathCost(int path_id,
                          const HGraphPtr& hgraph,
                          const Partitions& solution,
                          int v = -1,
                          int to_pid = -1) const;

  // Find all the boundary vertices. The boundary vertices will not include any
  // fixed vertices
  std::vector<int> FindBoundaryVertices(
      const HGraphPtr& hgraph,
      const Matrix<int>& net_degs,
      const std::vector<bool>& visited_vertices_flag) const;

  std::vector<int> FindBoundaryVertices(
      const HGraphPtr& hgraph,
      const Matrix<int>& net_degs,
      const std::vector<bool>& visited_vertices_flag,
      const std::vector<int>& solution,
      const std::pair<int, int>& partition_pair) const;

  std::vector<int> FindNeighbors(
      const HGraphPtr& hgraph,
      int vertex_id,
      const std::vector<bool>& visited_vertices_flag) const;

  std::vector<int> FindNeighbors(
      const HGraphPtr& hgraph,
      int vertex_id,
      const std::vector<bool>& visited_vertices_flag,
      const std::vector<int>& solution,
      const std::pair<int, int>& partition_pair) const;

  // Functions related to move a vertex and hyperedge
  // -----------------------------------------------------------
  // The most important function for refinent
  // If we want to update the score function for other purposes
  // we should update this function.
  // -----------------------------------------------------------
  // calculate the possible gain of moving a vertex
  // we need following arguments:
  // from_pid : from block id
  // to_pid : to block id
  // solution : the current solution
  // cur_paths_cost : current path cost
  // net_degs : current net degrees
  GainCell CalculateVertexGain(int v,
                               int from_pid,
                               int to_pid,
                               const HGraphPtr& hgraph,
                               const std::vector<int>& solution,
                               const std::vector<float>& cur_paths_cost,
                               const Matrix<int>& net_degs) const;

  // accept the vertex gain
  void AcceptVertexGain(const GainCell& gain_cell,
                        const HGraphPtr& hgraph,
                        float& total_delta_gain,
                        std::vector<bool>& visited_vertices_flag,
                        std::vector<int>& solution,
                        std::vector<float>& cur_paths_cost,
                        Matrix<float>& curr_block_balance,
                        Matrix<int>& net_degs) const;

  // restore the vertex gain
  void RollBackVertexGain(const GainCell& gain_cell,
                          const HGraphPtr& hgraph,
                          std::vector<bool>& visited_vertices_flag,
                          std::vector<int>& solution,
                          std::vector<float>& cur_paths_cost,
                          Matrix<float>& curr_block_balance,
                          Matrix<int>& net_degs) const;

  // check if we can move the vertex to some block
  bool CheckVertexMoveLegality(int v,         // vertex_id
                               int to_pid,    // to block id
                               int from_pid,  // from block id
                               const HGraphPtr& hgraph,
                               const Matrix<float>& curr_block_balance,
                               const Matrix<float>& upper_block_balance,
                               const Matrix<float>& lower_block_balance) const;

  // Calculate the possible gain of moving a entire hyperedge.
  // We can view the process of moving the vertices in hyperege
  // one by one, then restore the moving sequence to make sure that
  // the current status is not changed. Solution should not be const
  // calculate the possible gain of moving a hyperedge
  HyperedgeGainPtr CalculateHyperedgeGain(
      int hyperedge_id,
      int to_pid,
      const HGraphPtr& hgraph,
      std::vector<int>& solution,
      const std::vector<float>& cur_paths_cost,
      const Matrix<int>& net_degs) const;

  // check if we can move the hyperegde into some block
  bool CheckHyperedgeMoveLegality(
      int e,       // hyperedge id
      int to_pid,  // to block id
      const HGraphPtr& hgraph,
      const std::vector<int>& solution,
      const Matrix<float>& curr_block_balance,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance) const;

  // accpet the hyperedge gain
  void AcceptHyperedgeGain(const HyperedgeGainPtr& hyperedge_gain,
                           const HGraphPtr& hgraph,
                           float& total_delta_gain,
                           std::vector<int>& solution,
                           std::vector<float>& cur_paths_cost,
                           Matrix<float>& cur_block_balance,
                           Matrix<int>& net_degs) const;

  // Note that there is no RollBackHyperedgeGain
  // Because we only use greedy hyperedge refinement

  // user specified parameters
  const int num_parts_ = 2;  // number of blocks in the partitioning
  int refiner_iters_ = 2;    // number of refinement iterations

  // the cost for cutting a critical timing path once.  If a critical
  // path is cut by 3 times, the cost is defined as 3 *
  // path_wt_factor_ * weight_of_path
  const float path_wt_factor_ = 1.0;

  // the cost of introducing a snaking timing path, see our paper for
  // detailed explanation of snaking timing paths
  const float snaking_wt_factor_ = 1.0;

  // the maxinum number of vertices can be moved in each pass
  int max_move_ = 50;

  // default parameters
  // during partitioning, we may need to update the value
  // of refiner_iters_ and max_move_ for the coarsest hypergraphs
  const int refiner_iters_default_ = 2;
  const int max_move_default_ = 50;

  utl::Logger* logger_ = nullptr;
  EvaluatorPtr evaluator_ = nullptr;
};

}  // namespace par

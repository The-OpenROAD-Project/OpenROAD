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
#include <string>
#include <utility>
#include <vector>

#include "Hypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// The path related statistics
struct PathStats
{
  int tot_num_path = 0;              // total number of paths
  int tot_num_critical_path = 0;     // total number of critical timing paths
  int tot_num_noncritical_path = 0;  // total number of noncritical timing paths
  int worst_cut_critical_path = 0;   // worst number of cuts on all the paths
  float avg_cut_critical_path = 0;   // average number of cuts on each path
  int number_non2critical_path
      = 0;  // total number of noncritical paths become critical
  int worst_cut_non2critical_path = 0;  // worst number of cuts on noncritical
                                        // paths that become critical
  float avg_cut_non2critical_path
      = 0.0f;  // average number of cuts on noncritical paths that
               // become critical
};

// Partitions is the partitioning solution
using Partitions = std::vector<int>;

// PartitionToken is the metrics of a given partition
struct PartitionToken
{
  float cost;                   // cutsize
  Matrix<float> block_balance;  // balance for each block
};

// GoldenEvaluator
class GoldenEvaluator;
using EvaluatorPtr = std::shared_ptr<GoldenEvaluator>;

// ------------------------------------------------------------------------
// The implementation of GoldenEvaluator
// It's used to compute the basic properties of a partitioning solution
// ------------------------------------------------------------------------
class GoldenEvaluator
{
 public:
  // TODO: update the constructor
  GoldenEvaluator(
      int num_parts,
      const std::vector<float>&
          e_wt_factors,  // the factor for hyperedge weight
      const std::vector<float>& v_wt_factors,  // the factor for vertex weight
      const std::vector<float>&
          placement_wt_factors,   // the factor for placement info
      float net_timing_factor,    // the factor for hyperedge timing weight
      float path_timing_factor,   // weight for cutting a critical timing path
      float path_snaking_factor,  // snaking factor a critical timing path
      float timing_exp_factor,  // timing exponetial factor for normalized slack
      float extra_cut_delay,    // the extra delay introduced by a cut
      HGraphPtr timing_graph,   // the timing graph needed
      utl::Logger* logger);

  GoldenEvaluator(const GoldenEvaluator&) = delete;
  GoldenEvaluator(GoldenEvaluator&) = delete;
  virtual ~GoldenEvaluator() = default;

  // calculate the vertex distribution of each net
  Matrix<int> GetNetDegrees(const HGraphPtr& hgraph,
                            const Partitions& solution) const;

  // Get block balance
  Matrix<float> GetBlockBalance(const HGraphPtr& hgraph,
                                const Partitions& solution) const;

  // calculate timing cost of a path
  float GetPathTimingScore(int path_id, const HGraphPtr& hgraph) const;

  // calculate the cost of a path
  float CalculatePathCost(int path_id,
                          const HGraphPtr& hgraph,
                          const Partitions& solution) const;

  // get the cost of all the paths: include the timing part and snaking part
  std::vector<float> GetPathsCost(const HGraphPtr& hgraph,
                                  const Partitions& solution) const;

  // calculate the status of timing path cuts
  PathStats GetTimingCuts(const HGraphPtr& hgraph,
                          const std::vector<int>& solution) const;

  void PrintPathStats(const PathStats& path_stats) const;

  // Calculate the timing cost due to the slack of hyperedge itself
  float CalculateHyperedgeTimingCost(int e, const HGraphPtr& hgraph) const;

  // Calculate the cost of a hyperedge
  float CalculateHyperedgeCost(int e, const HGraphPtr& hgraph) const;

  // Calculate the summation of normalized vertex weights
  // connecting to the same hyperedge
  float CalculateHyperedgeVertexWtSum(int e, const HGraphPtr& hgraph) const;

  // calculate the hyperedge score. score / (hyperedge.size() - 1)
  float GetNormEdgeScore(int e, const HGraphPtr& hgraph) const;

  // calculate the vertex weight norm
  // This is usually used to sort the vertices
  float GetVertexWeightNorm(int v, const HGraphPtr& hgraph) const;

  // calculate the placement score between vertex v and u
  float GetPlacementScore(int v, int u, const HGraphPtr& hgraph) const;

  // get vertex weight summation
  std::vector<float> GetVertexWeightSum(const HGraphPtr& hgraph,
                                        const std::vector<int>& group) const;

  // get the fixed attribute of a group of vertices (maximum)
  int GetGroupFixedAttr(const HGraphPtr& hgraph,
                        const std::vector<int>& group) const;

  // get the community attribute of a group of vertices (maximum)
  int GetGroupCommunityAttr(const HGraphPtr& hgraph,
                            const std::vector<int>& group) const;

  // get the placement location
  std::vector<float> GetGroupPlacementLoc(const HGraphPtr& hgraph,
                                          const std::vector<int>& group) const;

  // Get average the placement location
  std::vector<float> GetAvgPlacementLoc(int v,
                                        int u,
                                        const HGraphPtr& hgraph) const;

  // calculate the average placement location
  std::vector<float> GetAvgPlacementLoc(
      const std::vector<float>& vertex_weight_a,
      const std::vector<float>& vertex_weight_b,
      const std::vector<float>& placement_loc_a,
      const std::vector<float>& placement_loc_b) const;

  // calculate the hyperedges being cut
  std::vector<int> GetCutHyperedges(const HGraphPtr& hgraph,
                                    const std::vector<int>& solution) const;

  // get the connectivity between blocks
  // std::map<std::pair<int, int>, float> : <block_id_a, block_id_b> : score
  // The score is the summation of hyperedges spanning block_id_a and block_id_b
  std::map<std::pair<int, int>, float> GetMatchingConnectivity(
      const HGraphPtr& hgraph,
      const std::vector<int>& solution) const;

  // calculate the statistics of a given partitioning solution
  // PartitionToken.first is the cutsize
  // PartitionToken.second is the balance constraint
  PartitionToken CutEvaluator(const HGraphPtr& hgraph,
                              const std::vector<int>& solution,
                              bool print_flag = false) const;

  // check the constraints
  // balance constraint, group constraint, fixed vertices constraint
  bool ConstraintAndCutEvaluator(
      const HGraphPtr& hgraph,
      const std::vector<int>& solution,
      float ub_factor,
      const std::vector<float>& base_balance,
      const std::vector<std::vector<int>>& group_attr,
      bool print_flag = false) const;

  // hgraph will be updated here
  // For timing-driven flow,
  // we need to convert the slack information to related weight
  // Basically we will transform the path_timing_attr_ to path_timing_cost_,
  // and transform hyperedge_timing_attr_ to hyperedge_timing_cost_.
  // Then overlay the path weighgts onto corresponding weights
  void InitializeTiming(const HGraphPtr& hgraph) const;

  // Update timing information of a hypergraph
  // For timing-driven flow,
  // we first need to update the timing information of all the hyperedges
  // and paths (path_timing_attr_ and hyperedge_timing_attr_),
  // i.e., introducing extra delay on the hyperedges being cut.
  // Then we call InitializeTiming to update the corresponding weights.
  // The timing_graph_ contains all the necessary information,
  // include the original slack for each path and hyperedge,
  // and the type of each vertex
  void UpdateTiming(const HGraphPtr& hgraph, const Partitions& solution) const;

  // Write the weighted hypergraph in hMETIS format
  void WriteWeightedHypergraph(const HGraphPtr& hgraph,
                               const std::string& file_name,
                               bool with_weight_flag = true) const;

  // Write the weighted hypergraph in hMETIS format
  void WriteIntWeightHypergraph(const HGraphPtr& hgraph,
                                const std::string& file_name) const;

 private:
  // user specified parameters
  const int num_parts_ = 2;            // number of blocks in the partitioning
  const float extra_cut_delay_ = 1.0;  // the extra delay introduced by a cut

  // This weight will be modifed when user call initial path
  std::vector<float>
      e_wt_factors_;  // the cost introduced by a cut hyperedge e is
                      // e_wt_factors dot_product hyperedge_weights_[e]
                      // this parameter is used by coarsening and partitioning

  const std::vector<float>
      v_wt_factors_;  // the ``weight'' of a vertex. For placement-driven
                      // coarsening, when we merge two vertices, we need to
                      // update the location of merged vertex based on the
                      // gravity center of these two vertices.  The weight of
                      // vertex v is defined as v_wt_factors dot_product
                      // vertex_weights_[v] this parameter is only used in
                      // coarsening

  const std::vector<float>
      placement_wt_factors_;  // the weight for placement information. For
                              // placement-driven coarsening, when we calculate
                              // the score for best-choice coarsening, the
                              // placement information also contributes to the
                              // score function. we prefer to merge two vertices
                              // which are adjacent physically the distance
                              // between u and v is defined as
                              // norm2(placement_attr[u] - placement_attr[v],
                              // placement_wt_factors_) this parameter is only
                              // used during coarsening

  const float net_timing_factor_
      = 1.0;  // the factor for hyperedge timing weight
  const float path_timing_factor_
      = 1.0;  // weight for cutting a critical timing path
  const float path_snaking_factor_
      = 1.0;  // snaking factor a critical timing path
  const float timing_exp_factor_ = 2.0;  // exponential factor

  HGraphPtr timing_graph_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace par

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
#include "Evaluator.h"

#include <algorithm>
#include <boost/range/iterator_range_core.hpp>
#include <cmath>
#include <fstream>
#include <functional>
#include <iomanip>
#include <ios>
#include <limits>
#include <map>
#include <numeric>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "Hypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

// ------------------------------------------------------------------------------
// Implementation of Golden Evaluator
// ------------------------------------------------------------------------------

namespace par {

using utl::PAR;

GoldenEvaluator::GoldenEvaluator(const int num_parts,
                                 const std::vector<float>& e_wt_factors,
                                 const std::vector<float>& v_wt_factors,
                                 const std::vector<float>& placement_wt_factors,
                                 const float net_timing_factor,
                                 const float path_timing_factor,
                                 const float path_snaking_factor,
                                 const float timing_exp_factor,
                                 const float extra_cut_delay,
                                 HGraphPtr timing_graph,
                                 utl::Logger* logger)
    : num_parts_(num_parts),
      extra_cut_delay_(extra_cut_delay),
      e_wt_factors_(e_wt_factors),
      v_wt_factors_(v_wt_factors),
      placement_wt_factors_(placement_wt_factors),
      net_timing_factor_(net_timing_factor),
      path_timing_factor_(path_timing_factor),
      path_snaking_factor_(path_snaking_factor),
      timing_exp_factor_(timing_exp_factor)
{
  timing_graph_ = std::move(timing_graph);
  logger_ = logger;
}

// calculate the vertex distribution of each net
Matrix<int> GoldenEvaluator::GetNetDegrees(const HGraphPtr& hgraph,
                                           const Partitions& solution) const
{
  Matrix<int> net_degs(hgraph->GetNumHyperedges(),
                       std::vector<int>(num_parts_, 0));
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    for (const int vertex_id : hgraph->Vertices(e)) {
      net_degs[e][solution[vertex_id]]++;
    }
  }
  return net_degs;
}

// Get block balance
Matrix<float> GoldenEvaluator::GetBlockBalance(const HGraphPtr& hgraph,
                                               const Partitions& solution) const
{
  Matrix<float> block_balance(
      num_parts_, std::vector<float>(hgraph->GetVertexDimensions(), 0.0));
  // update the block_balance
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    block_balance[solution[v]]
        = block_balance[solution[v]] + hgraph->GetVertexWeights(v);
  }
  return block_balance;
}

// calculate timing cost of a path
float GoldenEvaluator::GetPathTimingScore(int path_id,
                                          const HGraphPtr& hgraph) const
{
  if (hgraph->GetNumTimingPaths() <= 0
      || hgraph->GetTimingPathSlackSize() < hgraph->GetNumTimingPaths()
      || path_id >= hgraph->GetNumTimingPaths()) {
    debugPrint(logger_,
               PAR,
               "evaluation",
               1,
               "No timing-critical paths when calling GetPathTimingScore()!");
    return 0.0;
  }
  return std::pow(1.0 - hgraph->PathTimingSlack(path_id), timing_exp_factor_);
}

// calculate the cost of a path : including timing and snaking cost
float GoldenEvaluator::CalculatePathCost(int path_id,
                                         const HGraphPtr& hgraph,
                                         const Partitions& solution) const
{
  if (hgraph->GetNumTimingPaths() <= 0
      || hgraph->GetTimingPathCostSize() < hgraph->GetNumTimingPaths()
      || path_id >= hgraph->GetNumTimingPaths()) {
    debugPrint(logger_,
               PAR,
               "evaluation",
               1,
               "No timing-critical paths when calling CalculatePathsCost()!");
    return 0.0;
  }
  float cost = 0.0;
  std::vector<int>
      path;  // we must use vector here.  Becuase we need to calculate the
             // snaking factor represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  for (const int u : hgraph->PathVertices(path_id)) {
    const int block_id = solution[u];
    if (path.empty() || path.back() != block_id) {
      path.push_back(block_id);
      if (block_counter.find(block_id) != block_counter.end()) {
        block_counter[block_id] += 1;
      } else {
        block_counter[block_id] = 1;
      }
    }
  }
  // check if the entire path is within the block
  if (path.size() <= 1) {
    return cost;  // the path is fully within the block
  }
  // timing-related cost (basic path_cost * number of cut on the path)
  cost = path_timing_factor_ * (path.size() - 1)
         * hgraph->PathTimingCost(path_id);
  // get the snaking factor of the path (maximum repetition of block_id - 1)
  int snaking_factor = 0;
  for (auto& [block_id, count] : block_counter) {
    if (count > snaking_factor) {
      snaking_factor = count;
    }
  }
  cost += path_snaking_factor_ * static_cast<float>(snaking_factor - 1);
  return cost;
}

// Get Paths cost: include the timing part and snaking part
std::vector<float> GoldenEvaluator::GetPathsCost(
    const HGraphPtr& hgraph,
    const Partitions& solution) const
{
  std::vector<float> paths_cost;  // the path_cost for each path
  if (hgraph->GetNumTimingPaths() <= 0
      || hgraph->GetTimingPathCostSize() < hgraph->GetNumTimingPaths()) {
    debugPrint(logger_,
               PAR,
               "evaluation",
               1,
               "No timing-critical paths when calling GetPathsCost()");
    return paths_cost;
  }
  // check each timing path
  for (auto path_id = 0; path_id < hgraph->GetNumTimingPaths(); path_id++) {
    paths_cost.push_back(CalculatePathCost(path_id, hgraph, solution));
  }
  return paths_cost;
}

// calculate the status of timing path cuts
PathStats GoldenEvaluator::GetTimingCuts(const HGraphPtr& hgraph,
                                         const std::vector<int>& solution) const
{
  PathStats path_stats;  // create tha path related statistics
  if (hgraph->GetNumTimingPaths() <= 0) {
    debugPrint(logger_,
               PAR,
               "evaluation",
               1,
               "This no timing-critical paths when calling GetTimingCuts()");
    return path_stats;
  }

  // Initialize the initial values
  path_stats.tot_num_path = hgraph->GetNumTimingPaths();
  for (int i = 0; i < hgraph->GetNumTimingPaths(); ++i) {
    std::vector<int> block_path;  // It must be a vector
    for (const int v : hgraph->PathVertices(i)) {
      const int block_id = solution[v];
      if (block_path.empty() || block_path.back() != block_id) {
        block_path.push_back(block_id);
      }
    }
    const int cut_on_path = block_path.size() - 1;
    float path_slack = hgraph->PathTimingSlack(i);
    if (path_slack < 0) {  // critical path
      path_stats.tot_num_critical_path += 1;
      if (cut_on_path > 0) {
        path_stats.worst_cut_critical_path
            = std::max(cut_on_path, path_stats.worst_cut_critical_path);
        path_stats.avg_cut_critical_path += cut_on_path;
      }
    } else {  // noncritical path
      path_stats.tot_num_noncritical_path += 1;
      // update the path slack based on extra delay
      path_slack -= extra_cut_delay_ * cut_on_path;
      if (path_slack < 0.0f) {
        path_stats.number_non2critical_path += 1;
        path_stats.worst_cut_non2critical_path
            = std::max(cut_on_path, path_stats.worst_cut_non2critical_path);
        path_stats.avg_cut_non2critical_path += cut_on_path;
      }
    }
  }

  // normalization to calculate average
  if (path_stats.tot_num_critical_path > 0) {
    path_stats.avg_cut_critical_path
        = path_stats.avg_cut_critical_path / path_stats.tot_num_critical_path;
  }

  if (path_stats.number_non2critical_path > 0) {
    path_stats.avg_cut_non2critical_path
        = path_stats.avg_cut_non2critical_path
          / path_stats.number_non2critical_path;
  }

  return path_stats;
}

void GoldenEvaluator::PrintPathStats(const PathStats& path_stats) const
{
  logger_->report("\tTotal number of timing paths = {}",
                  path_stats.tot_num_path);
  logger_->report("\tTotal number of timing-critical paths = {}",
                  path_stats.tot_num_critical_path);
  logger_->report("\tTotal number of timing-noncritical paths = {}",
                  path_stats.tot_num_noncritical_path);
  logger_->report("\tThe worst number of cuts on timing-critical paths = {}",
                  path_stats.worst_cut_critical_path);
  logger_->report("\tThe average number of cuts on timing-critical paths = {}",
                  path_stats.avg_cut_critical_path);
  logger_->report(
      "\tTotal number of timing-noncritical to timing critical paths = {}",
      path_stats.number_non2critical_path);
  logger_->report(
      "\tThe worst number of cuts on timing-non2critical paths = {}",
      path_stats.worst_cut_non2critical_path);
  logger_->report(
      "\tThe average number of cuts on timing-non2critical paths = {}",
      path_stats.avg_cut_non2critical_path);
}

/*
// calculate the status of timing path cuts
// total cut, worst cut, average cut
std::tuple<int, int, float> GoldenEvaluator::GetTimingCuts(
    const HGraphPtr hgraph,
    const Partitions& solution) const
{
  if (hgraph->GetNumTimingPaths() <= 0) {
    logger_->warn(PAR, 142,
        "This no timing-critical paths when calling GetTimingCuts()");
    return std::make_tuple(0, 0, 0.0f);
  }
  int total_critical_paths_cut = 0;
  int worst_cut = 0;
  for (int i = 0; i < hgraph->GetNumTimingPaths(); ++i) {
    const int first_valid_entry = hgraph->vptr_p_[i];
    const int first_invalid_entry = hgraph->vptr_p_[i + 1];
    std::vector<int> block_path;  // It must be a vector
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int v = hgraph->vind_p_[j];
      const int block_id = solution[v];
      if (block_path.size() == 0 || block_path.back() != block_id) {
        block_path.push_back(block_id);
      }
    }
    const int cut_on_path = block_path.size() - 1;
    if (cut_on_path > 0) {
      worst_cut = std::max(cut_on_path, worst_cut);
      total_critical_paths_cut++;
    }
  }
  const float avg_cut
      = 1.0 * total_critical_paths_cut / hgraph->GetNumTimingPaths();
  return std::make_tuple(total_critical_paths_cut, worst_cut, avg_cut);
}
*/

// Calculate the timing cost due to the slack of hyperedge itself
float GoldenEvaluator::CalculateHyperedgeTimingCost(
    int e,
    const HGraphPtr& hgraph) const
{
  if (hgraph->HasTiming()) {
    return std::pow(1.0 - hgraph->GetHyperedgeTimingAttr(e),
                    timing_exp_factor_);
  }
  return 0.0;
}

// Calculate the cost of a hyperedge
float GoldenEvaluator::CalculateHyperedgeCost(int e,
                                              const HGraphPtr& hgraph) const
{
  // calculate the edge score
  float cost = std::inner_product(hgraph->GetHyperedgeWeights(e).begin(),
                                  hgraph->GetHyperedgeWeights(e).end(),
                                  e_wt_factors_.begin(),
                                  0.0);
  if (hgraph->HasTiming()) {
    // Note that hgraph->GetHyperedgeTimingCost(e) may be different from
    // the CalculateHyperedgeTimingCost(e, hgraph). Because this hyperedge may
    // belong to multiple paths, so we will add these timing related cost to
    // hgraph->GetHyperedgeTimingCost(e)
    cost += net_timing_factor_ * hgraph->GetHyperedgeTimingCost(e);
  }
  return cost;
}

// calculate the hyperedge score. score / (hyperedge.size() - 1)
float GoldenEvaluator::GetNormEdgeScore(int e, const HGraphPtr& hgraph) const
{
  const int he_size = hgraph->Vertices(e).size();
  if (he_size <= 1) {
    return 0.0;
  }
  return CalculateHyperedgeCost(e, hgraph) / (he_size - 1);
}

// Calculate the summation of normalized vertex weights
// connecting to the same hyperedge
float GoldenEvaluator::CalculateHyperedgeVertexWtSum(
    int e,
    const HGraphPtr& hgraph) const
{
  float weight = 0.0;
  for (const int vertex_id : hgraph->Vertices(e)) {
    weight += GetVertexWeightNorm(vertex_id, hgraph);
  }
  return weight;
}

// calculate the vertex weight norm
// This is usually used to sort the vertices
float GoldenEvaluator::GetVertexWeightNorm(int v, const HGraphPtr& hgraph) const
{
  return std::inner_product(hgraph->GetVertexWeights(v).begin(),
                            hgraph->GetVertexWeights(v).end(),
                            v_wt_factors_.begin(),
                            0.0f);
}

// calculate the placement score between vertex v and u
float GoldenEvaluator::GetPlacementScore(int v,
                                         int u,
                                         const HGraphPtr& hgraph) const
{
  const float dist = norm2(hgraph->GetPlacement(v) - hgraph->GetPlacement(u),
                           placement_wt_factors_);
  if (dist == 0.0) {
    return std::numeric_limits<float>::max() / 2.0;
  }
  return 1.0f / dist;
}

// Get average the placement location
std::vector<float>
GoldenEvaluator::GetAvgPlacementLoc(int v, int u, const HGraphPtr& hgraph) const
{
  const float v_weight = GetVertexWeightNorm(v, hgraph);
  const float u_weight = GetVertexWeightNorm(u, hgraph);
  const float weight_sum = v_weight + u_weight;

  return MultiplyFactor(hgraph->GetPlacement(v), v_weight / weight_sum)
         + MultiplyFactor(hgraph->GetPlacement(u), u_weight / weight_sum);
}

// calculate the average placement location
std::vector<float> GoldenEvaluator::GetAvgPlacementLoc(
    const std::vector<float>& vertex_weight_a,
    const std::vector<float>& vertex_weight_b,
    const std::vector<float>& placement_loc_a,
    const std::vector<float>& placement_loc_b) const
{
  const float a_weight = std::inner_product(vertex_weight_a.begin(),
                                            vertex_weight_a.end(),
                                            v_wt_factors_.begin(),
                                            0.0f);

  const float b_weight = std::inner_product(vertex_weight_b.begin(),
                                            vertex_weight_b.end(),
                                            v_wt_factors_.begin(),
                                            0.0f);

  const float weight_sum = a_weight + b_weight;
  return MultiplyFactor(placement_loc_a, a_weight / weight_sum)
         + MultiplyFactor(placement_loc_b, b_weight / weight_sum);
}

// get vertex weight summation
std::vector<float> GoldenEvaluator::GetVertexWeightSum(
    const HGraphPtr& hgraph,
    const std::vector<int>& group) const
{
  std::vector<float> group_weight(hgraph->GetPlacementDimensions(), 0.0f);
  for (const auto& v : group) {
    group_weight = group_weight + hgraph->GetVertexWeights(v);
  }
  return group_weight;
}

// get the fixed attribute of a group of vertices (maximum)
int GoldenEvaluator::GetGroupFixedAttr(const HGraphPtr& hgraph,
                                       const std::vector<int>& group) const
{
  int fixed_attr = -1;
  if (!hgraph->HasFixedVertices()) {
    return fixed_attr;
  }

  for (const auto& v : group) {
    fixed_attr = std::max(fixed_attr, hgraph->GetFixedAttr(v));
  }

  return fixed_attr;
}

// get the community attribute of a group of vertices (maximum)
int GoldenEvaluator::GetGroupCommunityAttr(const HGraphPtr& hgraph,
                                           const std::vector<int>& group) const
{
  int community_attr = -1;
  if (!hgraph->HasCommunity()) {
    return community_attr;
  }

  for (const auto& v : group) {
    community_attr = std::max(community_attr, hgraph->GetCommunity(v));
  }

  return community_attr;
}

// get the placement location
std::vector<float> GoldenEvaluator::GetGroupPlacementLoc(
    const HGraphPtr& hgraph,
    const std::vector<int>& group) const
{
  std::vector<float> group_weight(hgraph->GetPlacementDimensions(), 0.0f);
  std::vector<float> group_loc(hgraph->GetPlacementDimensions(), 0.0f);
  if (!hgraph->HasPlacement()) {
    return group_weight;
  }

  for (const auto& v : group) {
    group_loc = GetAvgPlacementLoc(group_weight,
                                   hgraph->GetVertexWeights(v),
                                   group_loc,
                                   hgraph->GetPlacement(v));
    group_weight = group_weight + hgraph->GetVertexWeights(v);
  }

  return group_weight;
}

// calculate the hyperedges being cut
std::vector<int> GoldenEvaluator::GetCutHyperedges(
    const HGraphPtr& hgraph,
    const std::vector<int>& solution) const
{
  std::vector<int> cut_hyperedges;
  // check the cutsize
  for (int e = 0; e < hgraph->GetNumHyperedges(); ++e) {
    const auto range = hgraph->Vertices(e);
    const int first_solution = solution[*range.begin()];
    for (const int vertex_id :
         boost::make_iterator_range(range.begin() + 1, range.end())) {
      if (solution[vertex_id] != first_solution) {
        cut_hyperedges.push_back(e);
        break;  // this net has been cut
      }
    }  // finish hyperedge e
  }
  return cut_hyperedges;
}

// Calculate the connectivity between blocks
// std::map<std::pair<int, int>, float> : <block_id_a, block_id_b> : score
// The score is the summation of hyperedges spanning block_id_a and block_id_b
std::map<std::pair<int, int>, float> GoldenEvaluator::GetMatchingConnectivity(
    const HGraphPtr& hgraph,
    const std::vector<int>& solution) const
{
  std::map<std::pair<int, int>, float> matching_connectivity;
  // the score between block_a and block_b is the same as
  // the score between block_b and block_a
  for (int block_a = 0; block_a < num_parts_; block_a++) {
    for (int block_b = block_a + 1; block_b < num_parts_; block_b++) {
      float score = 0.0;
      // check each hyperedge
      for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
        bool block_a_flag = false;  // the hyperedge intersects with block_a
        bool block_b_flag = false;  // the hyperedge intersects with block_b
        for (const int vertex_id : hgraph->Vertices(e)) {
          const int block_id = solution[vertex_id];
          if (block_a_flag == false && block_id == block_a) {
            block_a_flag = true;
          }
          if (block_b_flag == false && block_id == block_b) {
            block_b_flag = true;
          }
          if (block_a_flag == true && block_b_flag == true) {
            score += CalculateHyperedgeCost(e, hgraph);
            break;
          }
        }
      }
      matching_connectivity[std::pair<int, int>(block_a, block_b)] = score;
    }
  }
  return matching_connectivity;
}

// calculate the statistics of a given partitioning solution
// PartitionToken.first is the cutsize
// PartitionToken.second is the balance constraint
PartitionToken GoldenEvaluator::CutEvaluator(const HGraphPtr& hgraph,
                                             const std::vector<int>& solution,
                                             bool print_flag) const
{
  Matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  float edge_cost = 0.0;
  float path_cost = 0.0;
  // check the cutsize
  std::vector<int> cut_hyperedges = GetCutHyperedges(hgraph, solution);
  for (auto& e : cut_hyperedges) {
    edge_cost += CalculateHyperedgeCost(e, hgraph);
  }
  // check path related cost
  for (int path_id = 0; path_id < hgraph->GetNumTimingPaths(); path_id++) {
    // the path cost has been weighted
    path_cost += CalculatePathCost(path_id, hgraph, solution);
  }
  const float cost = edge_cost + path_cost;

  if (print_flag && logger_->debugCheck(PAR, "evaluation", 1)) {
    logger_->report("\nPrint Statistics for Partition\n");
    logger_->report("Cutcost : {}", cost);
    // print block balance
    const std::vector<float> tot_vertex_weights
        = hgraph->GetTotalVertexWeights();
    for (auto block_id = 0; block_id < num_parts_; block_id++) {
      std::string line
          = "Vertex balance of block_" + std::to_string(block_id) + " : ";
      for (auto dim = 0; dim < tot_vertex_weights.size(); dim++) {
        std::stringstream ss;  // for converting float to string
        ss << std::fixed << std::setprecision(5)
           << block_balance[block_id][dim] / tot_vertex_weights[dim] << "  ( "
           << block_balance[block_id][dim] << " )  ";
        line += ss.str() + "  ";
      }
      logger_->report(line);
    }  // finish block balance
  }

  return PartitionToken{cost, block_balance};
}

// check the constraints
// balance constraint, group constraint, fixed vertices constraint
bool GoldenEvaluator::ConstraintAndCutEvaluator(
    const HGraphPtr& hgraph,
    const std::vector<int>& solution,
    float ub_factor,
    const std::vector<float>& base_balance,
    const std::vector<std::vector<int>>& group_attr,
    bool print_flag) const
{
  auto solution_token = CutEvaluator(hgraph, solution, print_flag);
  // check block balance
  bool balance_satisfied_flag = true;
  const Matrix<float> upper_block_balance
      = hgraph->GetUpperVertexBalance(num_parts_, ub_factor, base_balance);
  const Matrix<float> lower_block_balance
      = hgraph->GetLowerVertexBalance(num_parts_, ub_factor, base_balance);
  for (int i = 0; i < num_parts_; i++) {
    if (solution_token.block_balance[i] > upper_block_balance[i]
        || solution_token.block_balance[i] < lower_block_balance[i]) {
      balance_satisfied_flag = false;
      break;
    }
  }

  // check group constraint
  bool group_satisified_flag = true;
  for (const auto& group : group_attr) {
    if (static_cast<int>(group.size()) <= 1) {
      continue;
    }
    int block_id = solution[group.front()];
    for (const auto& v : group) {
      if (solution[v] != block_id) {
        group_satisified_flag = false;
        break;
      }
    }
  }

  // check fixed vertices constraint
  bool fixed_satisfied_flag = true;
  if (hgraph->GetFixedAttrSize() == hgraph->GetNumVertices()) {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      if (hgraph->GetFixedAttr(v) > -1
          && hgraph->GetFixedAttr(v) != solution[v]) {
        fixed_satisfied_flag = false;
        break;
      }
    }
  }

  if (print_flag == true && logger_->debugCheck(PAR, "evaluation", 1)) {
    logger_->report("\nConstraints and Cut Evaluation\n");
    logger_->report("Satisfy the balance constraint : {}",
                    balance_satisfied_flag);
    logger_->report("Satisfy the group constraint : {}", group_satisified_flag);
    logger_->report("Satisfy the fixed vertices constraint : {}",
                    fixed_satisfied_flag);
  }

  return balance_satisfied_flag && group_satisified_flag
         && fixed_satisfied_flag;
}

// hgraph will be updated here
// For timing-driven flow,
// we need to convert the slack information to related weight
// Basically we will transform the path_timing_attr_ to path_timing_cost_,
// and transform hyperedge_timing_attr_ to hyperedge_timing_cost_.
// Then overlay the path weighgts onto corresponding weights
void GoldenEvaluator::InitializeTiming(const HGraphPtr& hgraph) const
{
  if (!hgraph->HasTiming()) {
    return;
  }

  // Step 1: calculate the path_timing_cost_
  hgraph->ResetPathTimingCost();
  for (int path_id = 0; path_id < hgraph->GetNumTimingPaths(); path_id++) {
    hgraph->SetPathTimingCost(path_id, GetPathTimingScore(path_id, hgraph));
  }

  // Step 2: calculate the hyperedge timing cost
  {
    std::vector<float> costs;
    costs.reserve(hgraph->GetNumHyperedges());
    for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
      costs.push_back(CalculateHyperedgeTimingCost(e, hgraph));
    }
    hgraph->SetHyperedgeTimingCost(std::move(costs));
  }

  // Step 3: traverse all the paths and lay the path weight on corresponding
  // hyperedges
  for (int path_id = 0; path_id < hgraph->GetNumTimingPaths(); path_id++) {
    for (const int e : hgraph->PathEdges(path_id)) {
      hgraph->AddHyperedgeTimingCost(e, hgraph->PathTimingCost(path_id));
    }
  }
}

// Update timing information of a hypergraph
// For timing-driven flow,
// we first need to update the timing information of all the hyperedges
// and paths (path_timing_attr_ and hyperedge_timing_attr_),
// i.e., introducing extra delay on the hyperedges being cut.
// Then we call InitializeTiming to update the corresponding weights.
// The timing_graph_ contains all the necessary information,
// include the original slack for each path and hyperedge,
// and the type of each vertex
void GoldenEvaluator::UpdateTiming(const HGraphPtr& hgraph,
                                   const Partitions& solution) const
{
  if (!hgraph->HasTiming()) {
    return;
  }

  // Here we need to update the path_timing_attr_ and hyperedge_timing_attr_ of
  // hgraph Step 1: update the hyperedge_timing_attr_ first identify all the
  // hyperedges being cut in the timing graph
  std::vector<int> cut_hyperedges = GetCutHyperedges(hgraph, solution);

  // Timing arc slacks store the updated slack for each hyperedge in the timing
  // graph instead of hgraph
  std::vector<float> timing_arc_slacks
      = timing_graph_->GetHyperedgeTimingAttr();
  /*
  for (const auto& e : cut_hyperedges) {
    for (const auto& arc_id : hgraph->hyperedge_arc_set_[e]) {
      timing_arc_slacks[arc_id] -= extra_cut_delay_;
    }
  }
  */

  // Function < return type (parameter  types) > functionName
  // Propogate the delay
  // Functions 1: propogate forward along critical paths
  std::function<void(int)> lambda_forward = [&](int e) -> void {
    const auto& e_slack = timing_arc_slacks[e];
    // check all the hyperedges connected to sink
    // for each hyperedge, the first vertex is the source
    // the remaining vertices are all sinks
    // It will stop if the sink vertex is a FF or IO
    for (const int v : timing_graph_->Vertices(e)) {
      if (timing_graph_->GetVertexType(v) != COMB_STD_CELL) {
        continue;  // the current vertex is port or seq_std_cell or macro
      }
      // find all the hyperedges connected to this hyperedge
      for (const int next_e : timing_graph_->Edges(v)) {
        if (timing_arc_slacks[next_e] > e_slack) {
          timing_arc_slacks[next_e] = e_slack;
          lambda_forward(next_e);  // propogate forward
        }
      }
    }
  };

  // Function 2: propogate backward along critical paths
  std::function<void(int)> lambda_backward = [&](int e) -> void {
    const auto& e_slack = timing_arc_slacks[e];
    // check all the hyperedges connected to sink
    // for each hyperedge, the first vertex is the source
    // the remaining vertices are all sinks
    // It will stop if the src vertex is a FF or IO
    // ignore single-vertex hyperedge
    const auto range = timing_graph_->Vertices(e);
    const int he_size = range.size();
    if (he_size <= 1) {
      return;  // this hyperedge (net) is invalid
    }
    // get the vertex id of source instance
    const int src_id = *range.begin();
    // Stop backward traversing if the current vertex is port or seq_std_cell or
    // macro
    if (timing_graph_->GetVertexType(src_id) != COMB_STD_CELL) {
      return;  // the current vertex is port or seq_std_cell or macro
    }
    // find all the hyperedges driving this vertex
    // find all the hyperedges connected to this hyperedge
    for (const int pre_e : timing_graph_->Edges(src_id)) {
      // check if the hyperedge drives src_id
      const auto pre_e_range = timing_graph_->Vertices(pre_e);
      const int pre_e_size = pre_e_range.size();
      if (pre_e_size <= 1) {
        return;  // this hyperedge (net) is invalid
      }
      // get the vertex id of source instance
      const int pre_src_id = *pre_e_range.begin();
      if (pre_src_id == src_id) {
        continue;  // this hyperedge has been considered in forward propogation
      }
      // backward traversing
      if (timing_arc_slacks[pre_e] > e_slack) {
        timing_arc_slacks[pre_e] = e_slack;
        lambda_backward(pre_e);  // propogate backward
      }
    }
  };

  // propagate the delay
  for (const auto& e : cut_hyperedges) {
    for (const auto& arc_id : hgraph->GetHyperedgeArcSet(e)) {
      timing_arc_slacks[arc_id] -= extra_cut_delay_;
      lambda_forward(arc_id);
      lambda_backward(arc_id);
    }
  }

  // update the hyperedge_timing_attr_
  hgraph->ResetHyperedgeTimingAttr();
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    for (const auto& arc_id : hgraph->GetHyperedgeArcSet(e)) {
      hgraph->SetHyperedgeTimingAttr(
          e,
          std::min(timing_arc_slacks[arc_id],
                   hgraph->GetHyperedgeTimingAttr(e)));
    }
  }

  // Step 2: update the path_timing_attr_.
  // the slack of a path is the worst slack of all its hyperedges
  hgraph->ResetPathTimingSlack();
  for (int path_id = 0; path_id < hgraph->GetNumTimingPaths(); path_id++) {
    float slack = std::numeric_limits<float>::max();
    for (const int e : hgraph->PathEdges(path_id)) {
      slack = std::min(slack, hgraph->GetHyperedgeTimingAttr(e));
    }
    hgraph->SetPathTimingSlack(path_id, slack);
  }

  // update the corresponding path and hyperedge timing weight
  InitializeTiming(hgraph);
}

// Write the weighted hypergraph in hMETIS format
void GoldenEvaluator::WriteWeightedHypergraph(const HGraphPtr& hgraph,
                                              const std::string& file_name,
                                              bool with_weight_flag) const
{
  std::ofstream file_output;
  file_output.open(file_name);
  if (with_weight_flag == true) {
    file_output << hgraph->GetNumHyperedges() << "  "
                << hgraph->GetNumVertices() << " 11" << std::endl;
  } else {
    file_output << hgraph->GetNumHyperedges() << "  "
                << hgraph->GetNumVertices() << std::endl;
  }
  // write hyperedge weight and hyperedge first
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    if (with_weight_flag == true) {
      file_output << CalculateHyperedgeCost(e, hgraph) << "  ";
    }
    for (const int vertex : hgraph->Vertices(e)) {
      file_output << vertex + 1 << " ";
    }
    file_output << std::endl;
  }
  // write vertex weight
  if (with_weight_flag == true) {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      file_output << GetVertexWeightNorm(v, hgraph) << std::endl;
    }
  }
  // close the file
  file_output.close();
}

// Write the weighted hypergraph in hMETIS format
void GoldenEvaluator::WriteIntWeightHypergraph(
    const HGraphPtr& hgraph,
    const std::string& file_name) const
{
  std::ofstream file_output;
  file_output.open(file_name);
  file_output << hgraph->GetNumHyperedges() << "  " << hgraph->GetNumVertices()
              << " 11" << std::endl;
  // write hyperedge weight and hyperedge first
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    file_output << std::round(CalculateHyperedgeCost(e, hgraph)) << "  ";
    for (const int vertex : hgraph->Vertices(e)) {
      file_output << vertex + 1 << " ";
    }
    file_output << std::endl;
  }
  // write vertex weight
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    file_output << std::round(GetVertexWeightNorm(v, hgraph)) << std::endl;
  }
  // close the file
  file_output.close();
}

}  // namespace par

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
#include "TPEvaluator.h"
#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

// ------------------------------------------------------------------------------
// Implementation of Golden Evaluator
// ------------------------------------------------------------------------------

namespace par {

// calculate the vertex distribution of each net
matrix<int> GoldenEvaluator::GetNetDegrees(const HGraph hgraph,
                                           const TP_partition& solution) const
{
  matrix<int> net_degs(hgraph->num_hyperedges_,
                       std::vector<int>(num_parts_, 0));
  for (int e = 0; e < hgraph->num_hyperedges_; e++) {
    for (int idx = hgraph->eptr_[e]; idx < hgraph->eptr_[e + 1]; idx++) {
      net_degs[e][solution[hgraph->eind_[idx]]]++;
    }
  }
  return net_degs;
}

// Get block balance
matrix<float> GoldenEvaluator::GetBlockBalance(const HGraph hgraph,
                                               const TP_partition& solution) const
{
  matrix<float> block_balance(
      num_parts_, std::vector<float>(hgraph->vertex_dimensions_, 0.0));
  // update the block_balance
  for (int v = 0; v < hgraph->num_vertices_; v++) {
    block_balance[solution[v]]
        = block_balance[solution[v]] + hgraph->vertex_weights_[v];
  }
  return block_balance;
}


// calculate timing cost of a path
float GoldenEvaluator::GetPathTimingScore(int path_id, const HGraph hgraph) const
{
  if (hgraph->num_timing_paths_ <= 0 || 
      hgraph->path_timing_attr_.size() < hgraph->num_timing_paths_ ||
      path_id >= hgraph->num_timing_paths_) 
  {
    logger_->report("[WARNING] This no timing-critical paths when calling GetPathTimingScore()");
    return 0.0;
  }
  return std::pow(1.0 - hgraph->path_timing_attr_[path_id], timing_exp_factor_);
}

// calculate the cost of a path : including timing and snaking cost
float GoldenEvaluator::CalculatePathCost(int path_id,
                                         const HGraph hgraph,
                                         const TP_partition& solution) const
{
  if (hgraph->num_timing_paths_ <= 0 || 
      hgraph->path_timing_cost_.size() < hgraph->num_timing_paths_ ||
      path_id >= hgraph->num_timing_paths_) 
  {
    logger_->report("[WARNING] This no timing-critical paths when calling CalculatePathsCost()");
    return 0.0;
  }
  float cost = 0.0;
  std::vector<int> path;    // we must use vector here.  Becuase we need to calculate the snaking factor
                            // represent the path in terms of block_id  
  std::map<int, int> block_counter;  // block_id counter
  for (auto idx = hgraph->vptr_p_[path_id]; idx < hgraph->vptr_p_[path_id + 1]; ++idx) {
    const int u = hgraph->vind_p_[idx];  // current vertex
    const int block_id = solution[u];
    if (path.size() == 0 || path.back() != block_id) {
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
    return cost; // the path is fully within the block
  }
  // timing-related cost (basic path_cost * number of cut on the path)
  cost = path_wt_factor_ * (path.size() - 1) * hgraph->path_timing_cost_[path_id];
  // get the snaking factor of the path (maximum repetition of block_id - 1)
  int snaking_factor = 0;
  for (auto& [block_id, count] : block_counter) {
    if (count > snaking_factor) {
      snaking_factor = count;
    }
  }
  cost += snaking_wt_factor_ * static_cast<float>(snaking_factor - 1);
  return cost;
}

// Get Paths cost: include the timing part and snaking part
std::vector<float> GoldenEvaluator::GetPathsCost(const HGraph hgraph, 
                                                 const TP_partition& solution) const
{
  std::vector<float> paths_cost; // the path_cost for each path
  if (hgraph->num_timing_paths_ <= 0 || 
      hgraph->path_timing_cost_.size() < hgraph->num_timing_paths_) 
  {
    logger_->report("[WARNING] This no timing-critical paths when calling GetPathsCost()");
    return paths_cost;
  }
  // check each timing path
  for (auto path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    paths_cost.push_back(CalculatePathCost(path_id, hgraph, solution));
  }
  return paths_cost;
}


// calculate the status of timing path cuts
// total cut, worst cut, average cut
std::tuple<int, int, float> GoldenEvaluator::GetTimingCuts(const HGraph hgraph,
                                                           const TP_partition& solution) const
{
  if (hgraph->num_timing_paths_ <= 0) {
    logger_->report("[WARNING] This no timing-critical paths when calling GetTimingCuts()");
    return std::make_tuple(0, 0, 0.0f);
  }
  int total_critical_paths_cut = 0;
  int worst_cut = 0;
  for (int i = 0; i < hgraph->num_timing_paths_; ++i) {
    const int first_valid_entry = hgraph->vptr_p_[i];
    const int first_invalid_entry = hgraph->vptr_p_[i + 1];
    std::vector<int> block_path; // It must be a vector
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
  const float avg_cut = 1.0 * total_critical_paths_cut / hgraph->num_timing_paths_;
  return std::make_tuple(total_critical_paths_cut, worst_cut, avg_cut);
}


// Calculate the timing cost due to the slack of hyperedge itself
float GoldenEvaluator::CalculateHyperedgeTimingCost(int e, const HGraph hgraph) const
{
  if (hgraph->timing_flag_ == true) {
    return std::pow(1.0 - hgraph->hyperedge_timing_attr_[e], timing_exp_factor_);         
  } else {
    return 0.0;
  }
}

// Calculate the cost of a hyperedge
float GoldenEvaluator::CalculateHyperedgeCost(int e, const HGraph hgraph) const {
  // calculate the edge score
  float cost = std::inner_product(hgraph->hyperedge_weights_[e].begin(),
                                  hgraph->hyperedge_weights_[e].end(),
                                  e_wt_factors_.begin(), 
                                  0.0);
  if (hgraph->timing_flag_ == true) {
    // Note that hgraph->hyperedge_timing_cost_[e] may be different from
    // the CalculateHyperedgeTimingCost(e, hgraph). Because this hyperedge may belong
    // to multiple paths, so we will add these timing related cost to hgraph->hyperedge_timing_cost_[e]
    cost += timing_factor_ * hgraph->hyperedge_timing_cost_[e]; 
  }
  return cost;   
}

// calculate the hyperedge score. score / (hyperedge.size() - 1)
float GoldenEvaluator::GetNormEdgeScore(int e, const HGraph hgraph) const
{
  const int he_size = hgraph->eptr_[e + 1] - hgraph->eptr_[e];
  if (he_size <= 1) {
    return 0.0;
  }
  return CalculateHyperedgeCost(e) / (he_size - 1);
}


// calculate the vertex weight norm
// This is usually used to sort the vertices
float GoldenEvaluator::GetVertexWeightNorm(int v, const HGraph hgraph) const 
{
  return std::inner_product(hgraph->vertex_weights_[v].begin(),
                            hgraph->vertex_weights_[v].end(),
                            v_wt_factors_.begin(), 
                            0.0f);
}
  

// calculate the placement score between vertex v and u
float GoldenEvaluator::GetPlacementScore(int v, int u, const HGraph hgraph) const
{
  return norm2(hgraph->placement_attr_[v], hgraph->placement_attr_[u], placement_wt_factors_);  
}


// Get average the placement location
float GoldenEvaluator::GetAvgPlacementLoc(int v, int u, const HGraph hgraph) const
{
  const float v_weight = GetVertexWeightNorm(v, hgraph);
  const float u_weight = GetVertexWeightNorm(u, hgraph);
  const float weight_sum = v_weight + u_weight;
  return hgraph->placement_attr_[v] * v_weight / weight_sum + 
         hgraph->placement_attr_[u] * u_weight / weight_sum;
}


// calculate the average placement location
std::vector<float> GoldenEvaluator::GetAvgPlacementLoc(const std::vector<float>& vertex_weight_a,
                                                       const std::vector<float>& vertex_weight_b,
                                                       const std::vector<float>& placement_loc_a,
                                                       const std::vector<float>& placement_loc_b) const
{
  const float a_weight = std::inner_product(vertex_weight_a.begin(),
                                            vertex_weight_a.end(),
                                            v_wt_factors_.begin(), 0.0f);
  
  const float b_weight = std::inner_product(vertex_weight_b.begin(),
                                            vertex_weight_b.end(),
                                            v_wt_factors_.begin(), 0.0f);


  const float weight_sum = a_weight + b_weight;
  return placement_loc_a * a_weight / weight_sum +
         placement_loc_b * b_weight / weight_sum;                          
}


// calculate the hyperedges being cut
std::vector<int> GoldenEvaluator::GetCutHyperedges(const HGraph hgraph, 
                                                   const std::vector<int>& solution) const
{
  std::vector<int> cut_hyperedges;
  // check the cutsize
  for (int e = 0; e < hgraph->num_hyperedges_; ++e) {
    for (int idx = hgraph->eptr_[e] + 1; idx < hgraph->eptr_[e + 1]; ++idx) {
      if (solution[hgraph->eind_[idx]] != solution[hgraph->eind_[idx - 1]]) {
        cut_hyperedges.push_back(e);
        break;  // this net has been cut
      }
    }  // finish hyperedge e
  }
  return cut_hyperedges;
}

// calculate the statistics of a given partitioning solution
// TP_partition_token.first is the cutsize
// TP_partition_token.second is the balance constraint
TP_partition_token GoldenEvaluator::CutEvaluator(const HGraph hgraph,
                                                 const std::vector<int>& solution,
                                                 bool print_flag) const                                          
{
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  float edge_cost = 0.0;
  float path_cost = 0.0;
  // check the cutsize
  std::vector<int> cut_hyperedges = GetCutHyperedges(hgraph, solution);
  for (auto& e : cut_hyperedges) {
    edge_cost += CalculateHyperedgeCost(e, hgraph, solution);
  }
  // check path related cost
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    // the path cost has been weighted
    path_cost += CalculatePathCost(path_id, hgraph, solution);
  }
  const float cost = edge_cost + path_cost;
  // print the statistics
  if (print_flag == true) {
    // print cost
    logger_->report("[Cutcost of partition : {}]", cost);
    // print block balance
    const std::vector<float> tot_vertex_weights = hgraph->GetTotalVertexWeights();
    for (auto block_id = 0; block_id < num_parts_; block_id++) {
      std::string line
          = "[Vertex balance of block_" + std::to_string(block_id) + " : ";
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

  return std::pair<float, matrix<float> >(cost, block_balance);
}


// hgraph will be updated here
// For timing-driven flow, 
// we need to convert the slack information to related weight
// Basically we will transform the path_timing_attr_ to path_timing_cost_,
// and transform hyperedge_timing_attr_ to hyperedge_timing_cost_.
// Then overlay the path weighgts onto corresponding weights
void GoldenEvaluator::InitializeTiming(HGraph hgraph) const
{
  if (hgraph->timing_flag_ == false) {
    return;
  }

  // Step 1: calculate the path_timing_cost_
  hgraph->path_timing_cost_.clear();
  hgraph->path_timing_cost_.reserve(hgraph->num_timing_paths_);
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    hgraph->path_timing_cost_.push_back(GetPathTimingScore(path_id, hgraph));
  } 

  // Step 2: calculate the hyperedge timing cost
  hgraph->hyperedge_timing_cost_.clear();
  hgraph->hyperedge_timing_cost_.reserve(hgraph->num_hyperedges_);
  for (int e = 0; e < hgraph->num_hyperedges_; e++) {
    hgraph->hyperedge_timing_cost_.push_back(CalculateHyperedgeTimingCost(e, hgraph));
  }

  // Step 3: traverse all the paths and lay the path weight on corresponding hyperedges
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    for (int idx = hgraph->eptr_p_[path_id]; idx < hgraph->eptr_p_[path_id++]; idx++) {
      const int e = hgraph->eind_p_[idx];
      hgraph->hyperedge_timing_cost_[e] += hgraph->path_timing_cost_[path_id]; 
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
void GoldenEvaluator::UpdateTiming(HGraph hgraph, const TP_partition& solution) const
{
  if (hgraph->timing_flag_ == false) {
    return;
  }
  // Here we need to update the path_timing_attr_ and hyperedge_timing_attr_ of hgraph
  // Step 1: update the hyperedge_timing_attr_ first
  // identify all the hyperedges being cut in the timing graph
  std::vector<int> cut_hyperedges = GetCutHyperedges(hgraph, solution);
  std::vector<int> timing_arc_slacks = timing_graph_->hyperedge_timing_attr_;
  for (const auto& e : cut_hyperedges) {
    for (const auto& arc_id : hgraph->hyperedge_arc_set_[e]) {
      timing_arc_slacks[arc_id] -= extra_cut_delay_; 
    }
  }

  // Propogate the delay 
  // Functions 1: propogate forward along critical paths
  auto lambda_forward = [&](int e) -> void {
    const auto& e_slack = timing_arc_slacks[e];
    // check all the hyperedges connected to sink
    // for each hyperedge, the first vertex is the source
    // the remaining vertices are all sinks
    // It will stop if the sink vertex is a FF or IO
    for (int idx = timing_graph_->eptr_[e] + 1;  idx < timing_graph_->eptr_[e + 1]; idx++) {
      const int v = timing_graph_->eind_[idx];
      if (timing_graph_->vertex_types_[v] != COMB_STD_CELL) {
        continue; // the current vertex is port or seq_std_cell or macro
      } 
      // find all the hyperedges connected to this hyperedge
      for (int e_idx = timing_graph_->vptr_[v]; e_idx < timing_graph_->vptr_[v + 1]; e_idx++) {
        const int next_e = timing_graph_->vind_[e_idx];
        if (timing_arc_slacks[next_e] > e_slack) {
          timing_arc_slacks[next_e] = e_slack;
          lambda_forward(next_e); // propogate forward
        }
      }
    }
  };

  // Function 2: propogate backward along critical paths
  auto lambda_backward = [&](int e) -> void {
    const auto& e_slack = timing_arc_slacks[e];
    // check all the hyperedges connected to sink
    // for each hyperedge, the first vertex is the source
    // the remaining vertices are all sinks
    // It will stop if the src vertex is a FF or IO
    // ignore single-vertex hyperedge
    const int he_size = timing_graph_->eptr_[e + 1] - timing_graph_->eptr_[e];
    if (he_size <= 1) {
      return; // this hyperedge (net) is invalid
    }        
    // get the vertex id of source instance
    const int src_id = timing_graph_->eind_[timing_graph_->eptr_[e]];
    // Stop backward traversing if the current vertex is port or seq_std_cell or macro
    if (timing_graph_->vertex_types_[src_id] != COMB_STD_CELL) {
      return; // the current vertex is port or seq_std_cell or macro
    } 
    // find all the hyperedges driving this vertex
    // find all the hyperedges connected to this hyperedge
    for (int e_idx = timing_graph_->vptr_[src_id]; e_idx < timing_graph_->vptr_[src_id + 1]; e_idx++) {
      const int pre_e = timing_graph_->vind_[e_idx];
      // check if the hyperedge drives src_id
      const int pre_e_size = timing_graph_->eptr_[pre_e + 1] - timing_graph_->eptr_[pre_e];
      if (pre_e_size <= 1) {
        return; // this hyperedge (net) is invalid
      }
      // get the vertex id of source instance
      const int pre_src_id = timing_graph_->eind_[timing_graph_->eptr_[pre_e]];
      if (pre_src_id == src_id) {
        continue; // this hyperedge has been considered in forward propogation
      }     
      // backward traversing
      if (timing_arc_slacks[pre_e] > e_slack) {
        timing_arc_slacks[pre_e] = e_slack;
        lambda_backward(pre_e); // propogate backward
      }
    }
  };    

  // propagate the delay
  for (const auto& e : cut_hyperedges) {
    for (const auto& arc_id : hgraph->hyperedge_arc_set_[e]) {
      lambda_forward(arc_id);
      lambda_backward(arc_id);
    }
  }

  // update the hyperedge_timing_attr_
  std::fill(hgraph->hyperedge_timing_attr_.begin(),
            hgraph->hyperedge_timing_attr_.end(),
            std::numeric_limits<float>::max());
  for (const auto& e : cut_hyperedges) {
    for (const auto& arc_id : hgraph->hyperedge_arc_set_[e]) {
      hgraph->hyperedge_timing_attr_[e] = 
            std::min(timing_arc_slacks[arc_id], hgraph->hyperedge_timing_attr_[e]);
    }
  }

  // Step 2: update the path_timing_attr_.
  // the slack of a path is the worst slack of all its hyperedges
  hgraph->path_timing_attr_.clear();
  hgraph->path_timing_attr_.reserve(hgraph->num_timing_paths_);
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    float slack = std::numeric_limits<float>::max();
    for (int idx = hgraph->eptr_p_[path_id]; idx < hgraph->eptr_p_[path_id + 1]; idx++) {
      const int e = hgraph->eind_p_[idx];
      slack = std::min(slack, hgraph->hyperedge_timing_attr_[e]);
    }
    hgraph->path_timing_attr_.push_back(slack);
  }

  // update the corresponding path and hyperedge timing weight
  InitializeTiming(hgraph);
}

}  // namespace par

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
#include "TPRefiner.h"

#include <ortools/base/commandlineflags.h>

#include "TPHypergraph.h"
#include "Utilities.h"

// edited by Zhiang (20230206)
// removed CPLEX
//#include "ilcplex/cplex.h"
//#include "ilcplex/ilocplex.h"
//#include <ortools/base/init_google.h>
#include <ortools/base/logging.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/linear_solver/linear_solver.pb.h>

#include "ortools/base/logging.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "utl/Logger.h"

namespace par {

using operations_research::MPConstraint;
using operations_research::MPObjective;
using operations_research::MPSolver;
using operations_research::MPVariable;
using operations_research::sat::BoolVar;
using operations_research::sat::CpModelBuilder;
using operations_research::sat::CpSolverResponse;
using operations_research::sat::CpSolverStatus;
using operations_research::sat::LinearExpr;
using operations_research::sat::SolutionBooleanValue;
using operations_research::sat::Solve;

matrix<int> TPrefiner::GetNetDegrees(const HGraph hgraph,
                                     TP_partition& solution)
{
  matrix<int> net_degs(hgraph->num_hyperedges_,
                       std::vector<int>(num_parts_, 0));
  for (int e = 0; e < hgraph->num_hyperedges_; ++e) {
    const int he_size = hgraph->eptr_[e + 1] - hgraph->eptr_[e];
    if (he_size > thr_he_size_skip_) {
      continue;
    }
    for (int idx = hgraph->eptr_[e]; idx < hgraph->eptr_[e + 1]; ++idx) {
      ++net_degs[e][solution[hgraph->eind_[idx]]];
    }
  }
  return net_degs;
}

std::vector<int> TPrefiner::FindBoundaryVertices(
    const HGraph hgraph,
    matrix<int>& net_degs,
    std::pair<int, int>& partition_pair)
{
  std::set<int> boundary_set;
  std::vector<bool> boundary_net_flag(hgraph->num_hyperedges_, false);
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    if (net_degs[i][partition_pair.first] > 0
        && net_degs[i][partition_pair.second] > 0) {
      boundary_net_flag[i] = true;
    }
  }
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    const int first_valid_entry = hgraph->vptr_[i];
    const int first_invalid_entry = hgraph->vptr_[i + 1];
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int h_id = hgraph->vind_[j];
      if (boundary_net_flag[h_id] == true) {
        boundary_set.insert(i);
        break;
      }
    }
  }

  std::vector<int> boundary_vertices(boundary_set.begin(), boundary_set.end());
  return boundary_vertices;
}

void TPrefiner::FindNeighbors(const HGraph hgraph,
                              int& vertex,
                              std::pair<int, int>& partition_pair,
                              TP_partition& solution,
                              std::set<int>& neighbors,
                              bool k_flag)
{
  const int first_valid_entry = hgraph->vptr_[vertex];
  const int first_invalid_entry = hgraph->vptr_[vertex + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    const int first_valid_entry_he = hgraph->eptr_[he];
    const int first_invalid_entry_he = hgraph->eptr_[he + 1];
    for (int j = first_valid_entry_he; j < first_invalid_entry_he; ++j) {
      const int v = hgraph->eind_[j];
      if (k_flag == false) {
        if (v == vertex || GetVisitStatus(v) == true
            || (solution[v] != partition_pair.first
                && solution[v] != partition_pair.second)) {
          continue;
        }
        neighbors.insert(v);
      } else {
        if (v == vertex || GetVisitStatus(v) == true) {
          continue;
        }
        neighbors.insert(v);
      }
    }
  }
}

TP_partition_token TPrefiner::CutEvaluator(const HGraph hgraph,
                                           std::vector<int>& solution)
{
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  float edge_cost = 0.0;
  float timing_cost = 0.0;
  // check the cutsize
  for (int e = 0; e < hgraph->num_hyperedges_; ++e) {
    for (int idx = hgraph->eptr_[e] + 1; idx < hgraph->eptr_[e + 1]; ++idx) {
      if (solution[hgraph->eind_[idx]] != solution[hgraph->eind_[idx - 1]]) {
        edge_cost += std::inner_product(hgraph->hyperedge_weights_[e].begin(),
                                        hgraph->hyperedge_weights_[e].end(),
                                        e_wt_factors_.begin(),
                                        0.0);
        break;  // this net has been cut
      }
    }  // finish hyperedge e
  }

  // check timing paths
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    timing_cost = CalculatePathCost(path_id, hgraph, solution);
  }
  float edge_cost_factor = 1.0;
  float timing_cost_factor = 1.0;
  float cost = edge_cost_factor * edge_cost + timing_cost_factor * timing_cost;

  return std::pair<float, std::vector<std::vector<float>>>(cost, block_balance);
}

std::pair<int, int> TPrefiner::GetTimingCuts(const HGraph hgraph,
                                             std::vector<int>& solution)
{
  int total_critical_paths_cut = 0;
  int worst_cut = -std::numeric_limits<int>::max();
  for (int i = 0; i < hgraph->num_timing_paths_; ++i) {
    const int first_valid_entry = hgraph->vptr_p_[i];
    const int first_invalid_entry = hgraph->vptr_p_[i + 1];
    std::vector<int> block_path;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      int v = hgraph->vind_p_[j];
      int block_id = solution[v];
      if (block_path.size() == 0 || block_path.back() != block_id) {
        block_path.push_back(block_id);
      }
    }
    int cut_on_path = block_path.size() - 1;
    if (cut_on_path > worst_cut) {
      worst_cut = cut_on_path;
    }
    if (cut_on_path > 0) {
      ++total_critical_paths_cut;
    }
  }
  return std::make_pair(total_critical_paths_cut, worst_cut);
}

// Get block balance
matrix<float> TPrefiner::GetBlockBalance(const HGraph hgraph,
                                         TP_partition& solution)
{
  matrix<float> block_balance(
      num_parts_, std::vector<float>(hgraph->vertex_dimensions_, 0.0));
  if (hgraph->num_vertices_ != static_cast<int>(solution.size()))
    return block_balance;
  // check if the solution is valid
  for (auto block_id : solution)
    if (block_id < 0 || block_id >= num_parts_)
      return block_balance;
  // update the block_balance
  for (int v = 0; v < hgraph->num_vertices_; v++)
    block_balance[solution[v]]
        = block_balance[solution[v]] + hgraph->vertex_weights_[v];
  return block_balance;
}

float TPrefiner::CalculatePathCost(int path_id,
                                   const HGraph hgraph,
                                   const TP_partition& solution,
                                   int v,
                                   int to_pid)
{
  float cost = 0.0;  // cost for current path
  if (hgraph->num_timing_paths_ == 0)
    return cost;  // no timing paths

  std::vector<int> path;             // represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  int block_id;
  for (auto idx = hgraph->vptr_p_[path_id]; idx < hgraph->vptr_p_[path_id + 1];
       ++idx) {
    const int u = hgraph->vind_p_[idx];  // current vertex
    if (to_pid == -1) {
      block_id = solution[u];
    } else {
      block_id = (u == v) ? to_pid : solution[u];
    }
    if (path.size() == 0 || path.back() != block_id) {
      path.push_back(block_id);
      if (block_counter.find(block_id) != block_counter.end())
        block_counter[block_id] += 1;
      else
        block_counter[block_id] = 1;
    }
  }

  if (path.size() <= 1) {
    return cost;
  }

  // check how many times the path is being cut by the partition
  /*cost = path_wt_factor_ * static_cast<float>(path.size() - 1)
   * hgraph->timing_attr_[path_id];*/

  cost = path_wt_factor_
         * std::pow(hgraph->timing_attr_[path_id],
                    2.0 * static_cast<float>(path.size() - 1));
  // cost = path_wt_factor_ * static_cast<float>(path.size() - 1);

  // get the snaking factor of the path (maximum repetition of block_id - 1)
  int snaking_factor = 0;
  for (auto [block_id, count] : block_counter)
    if (count > snaking_factor)
      snaking_factor = count;
  cost += snaking_wt_factor_ * static_cast<float>(snaking_factor - 1);

  return cost;
}

bool TPrefiner::CheckBoundaryVertex(const HGraph hgraph,
                                    const int& v,
                                    const std::pair<int, int> partition_pair,
                                    const matrix<int>& net_degs) const
{
  const int first_valid_entry = hgraph->vptr_[v];
  const int first_invalid_entry = hgraph->vptr_[v + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    if (net_degs[he][partition_pair.first] > 0
        && net_degs[he][partition_pair.second] > 0)
      return true;
  }
  return false;
}

int TPrefiner::GetPathCuts(int pathid,
                           const HGraph hgraph,
                           std::vector<int>& solution)
{
  std::vector<int> path;             // represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  for (auto idx = hgraph->vptr_p_[pathid]; idx < hgraph->vptr_p_[pathid + 1];
       ++idx) {
    const int u = hgraph->vind_p_[idx];  // current vertex
    const int block_id = solution[u];
    if (path.size() == 0 || path.back() != block_id) {
      path.push_back(block_id);
      if (block_counter.find(block_id) != block_counter.end())
        block_counter[block_id] += 1;
      else
        block_counter[block_id] = 1;
    }
  }
  return path.size() - 1;
}

void TPrefiner::UpdateNeighboringPaths(
    const std::pair<int, int>& partition_pair,
    const std::set<int>& neighbors,
    const HGraph hgraph,
    const TP_partition& solution,
    std::vector<float>& paths_cost)
{
  if (hgraph->num_timing_paths_ > 0) {
    std::set<int> neighboring_paths;
    for (auto& v : neighbors) {
      const int first_valid_entry = hgraph->pptr_v_[v];
      const int first_invalid_entry = hgraph->pptr_v_[v + 1];
      for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
        int path = hgraph->pind_v_[i];
        neighboring_paths.insert(path);
      }
    }
    for (auto& path : neighboring_paths) {
      paths_cost[path] = CalculatePathCost(path, hgraph, solution);
    }
  }
}

// Generate paths as edges
void TPrefiner::GeneratePathsAsEdges(const HGraph hgraph)
{
  paths_.resize(hgraph->num_timing_paths_);
  for (int i = 0; i < hgraph->num_timing_paths_; ++i) {
    const int first_valid_entry = hgraph->vptr_p_[i];
    const int first_invalid_entry = hgraph->vptr_p_[i + 1];
    std::set<int> path_hyperedges;
    std::map<int, int> path_edges;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int v = hgraph->vind_p_[j];
      const int first_valid_entry_he = hgraph->vptr_[v];
      const int first_invalid_entry_he = hgraph->vptr_[v + 1];
      for (int k = first_valid_entry_he; k < first_invalid_entry_he; ++k) {
        int he = hgraph->vind_[k];
        path_hyperedges.insert(he);
      }
    }
    std::vector<int> hes(path_hyperedges.begin(), path_hyperedges.end());
    paths_[i] = hes;
  }
}

inline void TPrefiner::InitPathCuts(const HGraph hgraph,
                                    std::vector<int>& path_cuts,
                                    std::vector<int>& solution)
{
  for (int i = 0; i < hgraph->timing_attr_.size(); ++i) {
    path_cuts[i] = GetPathCuts(i, hgraph, solution);
  }
}

inline void TPrefiner::InitPaths(const HGraph hgraph,
                                 std::vector<float>& path_cost,
                                 std::vector<int>& solution)
{
  // rescale hyperedge weights to original weights so that these get
  // recalculated according to cuts on path graphs
  for (int i = 0; i < hgraph->timing_attr_.size(); ++i) {
    ReweighTimingWeights(i, hgraph, solution, path_cost);
  }
}

// Check if the path cost has changed in the past iteration
void TPrefiner::ReweighTimingWeights(int path_id,
                                     const HGraph hgraph,
                                     std::vector<int>& solution,
                                     std::vector<float>& path_cost)
{
  if (hgraph->num_timing_paths_ == 0)
    return;  // no timing paths

  // If the slack on the path is positive, then it is not critical and won't be
  // considered in our cost calculations
  if (hgraph->timing_attr_[path_id] < 1.0) {
    return;
  }

  std::vector<int> path;             // represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  for (auto idx = hgraph->vptr_p_[path_id]; idx < hgraph->vptr_p_[path_id + 1];
       ++idx) {
    const int u = hgraph->vind_p_[idx];  // current vertex
    const int block_id = solution[u];
    if (path.size() == 0 || path.back() != block_id) {
      path.push_back(block_id);
      if (block_counter.find(block_id) != block_counter.end())
        block_counter[block_id] += 1;
      else
        block_counter[block_id] = 1;
    }
  }
  // This path is cut again
  // On further cutting the timing critical path the edges lying along the path
  // should be reweighed with more delta weights
  if (path.size() > 1) {
    // check how many times the path is being cut by the partition
    // and recalculate the weight on the path
    // path_cost = path_cost_factor * modified_slack * number_of_cuts_on_path
    /*float cost = path_wt_factor_ * hgraph->timing_attr_[path_id]
     * static_cast<float>(path.size() - 1);*/
    float cost = path_wt_factor_
                 * std::pow(hgraph->timing_attr_[path_id],
                            2.0 * static_cast<float>(path.size() - 1));
    if (cost > 1.0) {
      // Each hyperedge weights will get updated based on whether it is cut or
      // not
      std::vector<float> cost_vector(hgraph->hyperedge_dimensions_, cost);
      /*for (auto& he : paths_[path_id]) {
        // account for path sharing
        hgraph->hyperedge_weights_[he]
            = hgraph->hyperedge_weights_[he] + cost_vector;
      }*/
    }

    // get the snaking factor of the path (maximum repetition of block_id - 1)
    int snaking_factor = 0;
    for (auto [block_id, count] : block_counter)
      if (count > snaking_factor)
        snaking_factor = count;
    cost += snaking_wt_factor_ * static_cast<float>(snaking_factor - 1);
    path_cost[path_id] = cost;
  }
}

TP_gain_cell TPrefiner::CalculateGain(int v,
                                      int from_pid,
                                      int to_pid,
                                      const HGraph hgraph,
                                      const std::vector<int>& solution,
                                      const std::vector<float>& cur_path_cost,
                                      const matrix<int>& net_degs)
{
  float cut_score = 0.0;
  float timing_score = 0.0;
  std::map<int, float> path_cost;  // map path_id to latest score
  if (from_pid == to_pid)
    return std::shared_ptr<VertexGain>(new VertexGain(v, 0.0, path_cost));
  // define two lambda function
  // 1) for checking connectivity
  // 2) for calculating the score of a hyperedge
  // function : check the connectivity for the hyperedge
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto& num_v : net_degs[e])
      if (num_v > 0)
        connectivity++;
    return connectivity;
  };
  // function : calculate the score for the hyperedge
  auto GetHyperedgeScore = [&](int e) {
    return std::inner_product(hgraph->hyperedge_weights_[e].begin(),
                              hgraph->hyperedge_weights_[e].end(),
                              e_wt_factors_.begin(),
                              0.0);
  };

  // traverse all the hyperedges connected to v
  const int first_valid_entry = hgraph->vptr_[v];
  const int first_invalid_entry = hgraph->vptr_[v + 1];
  for (auto e_idx = first_valid_entry; e_idx < first_invalid_entry; e_idx++) {
    const int e = hgraph->vind_[e_idx];  // hyperedge id
    const int connectivity = GetConnectivity(e);
    const float e_score = GetHyperedgeScore(e);
    const int he_size = hgraph->eptr_[e + 1] - hgraph->eptr_[e];

    if (he_size > thr_he_size_skip_) {
      continue;
    }
    if (connectivity == 1 && net_degs[e][from_pid] > 1) {
      // move from_pid to to_pid will have negative score
      cut_score -= e_score;
    } else if (connectivity == 2 && net_degs[e][from_pid] == 1
               && net_degs[e][to_pid] > 0) {
      // move from_pid to to_pid will increase the score
      cut_score += e_score;
    }
  }
  // check the timing path
  if (hgraph->num_timing_paths_ > 0) {
    for (auto p_idx = hgraph->pptr_v_[v]; p_idx < hgraph->pptr_v_[v + 1];
         ++p_idx) {
      const int path_id = hgraph->pind_v_[p_idx];
      // Get updated path costs if vertex is moved to a different partition
      const float cost
          = CalculatePathCost(path_id, hgraph, solution, v, to_pid);
      // gain accomodates for the change in the cost of the timing path
      timing_score += (cur_path_cost[path_id] - cost);
    }
  }
  // Comment from Zhiang
  // To do list (20230204) : cut_cost_factor and timing_cost_factor should be
  // defined as parameters This should be defined as parameters
  const float cut_cost_factor = 1.0;
  const float timing_cost_factor = 1.0;
  float score = cut_cost_factor * cut_score + timing_cost_factor * timing_score;
  return std::shared_ptr<VertexGain>(
      new VertexGain(v, score, solution[v], path_cost));
}

bool TPrefiner::CheckLegality(const HGraph hgraph,
                              int to,
                              std::shared_ptr<VertexGain> v,
                              const matrix<float>& curr_block_balance,
                              const matrix<float>& max_block_balance)
{
  if (GetVisitStatus(v->GetVertex()) == true) {
    return false;
  }
  std::vector<float> total_wt
      = curr_block_balance[to] + hgraph->vertex_weights_[v->GetVertex()];
  if (total_wt <= max_block_balance[to]) {
    return true;
  }
  return false;
}

inline void TPpriorityQueue::HeapifyUp(int index)
{
  while (index > 0
         && vertices_[Parent(index)]->GetGain() < vertices_[index]->GetGain()) {
    // Update the map
    auto& parent_heap_element = vertices_[Parent(index)];
    auto& child_heap_element = vertices_[index];
    vertices_map_[child_heap_element->GetVertex()] = Parent(index);
    vertices_map_[parent_heap_element->GetVertex()] = index;
    // Swap the elements
    std::swap(parent_heap_element, child_heap_element);
    // Next iteration
    index = Parent(index);
  }
}

void TPpriorityQueue::HeapifyDown(int index)
{
  int max_index = index;
  int left_child = LeftChild(index);
  if (left_child < total_elements_
      && vertices_[left_child]->GetGain() > vertices_[max_index]->GetGain()) {
    max_index = left_child;
  }
  int right_child = RightChild(index);
  if (right_child < total_elements_
      && vertices_[right_child]->GetGain() > vertices_[max_index]->GetGain()) {
    max_index = right_child;
  } else if (right_child < total_elements_
             && vertices_[right_child]->GetGain()
                    == vertices_[max_index]->GetGain()) {
    // If the gains are equal then pick the vertex with the smaller weight
    // The hope is doing this will incentivize in preventing corking effect
    // The alternative option is keep the ordering of insertion by the PQ
    if (hypergraph_->vertex_weights_[vertices_[right_child]->GetVertex()]
        < hypergraph_->vertex_weights_[vertices_[max_index]->GetVertex()]) {
      max_index = right_child;
    }
  }
  if (index != max_index) {
    auto& left_heap_element = vertices_[index];
    auto& right_heap_element = vertices_[max_index];
    // Update the map
    vertices_map_[left_heap_element->GetVertex()] = max_index;
    vertices_map_[right_heap_element->GetVertex()] = index;
    // Swap the elements
    std::swap(left_heap_element, right_heap_element);
    // Next recursive iteration
    HeapifyDown(max_index);
  }
}

void TPpriorityQueue::InsertIntoPQ(std::shared_ptr<VertexGain> vertex)
{
  total_elements_++;
  vertices_.push_back(vertex);
  vertices_map_[vertex->GetVertex()] = total_elements_ - 1;
  HeapifyUp(total_elements_ - 1);
}

std::shared_ptr<VertexGain> TPpriorityQueue::ExtractMax()
{
  auto max_element = vertices_.front();
  vertices_[0] = vertices_[total_elements_ - 1];
  vertices_map_[vertices_[total_elements_ - 1]->GetVertex()] = 0;
  total_elements_--;
  vertices_.pop_back();
  HeapifyDown(0);
  // Set location of this vertex to -1 in the map
  vertices_map_[max_element->GetVertex()] = -1;
  return max_element;
}

void TPpriorityQueue::ChangePriority(int index, float priority)
{
  float old_priority = vertices_[index]->GetGain();
  vertices_[index]->SetGain(priority);
  if (priority > old_priority) {
    HeapifyUp(index);
  } else {
    HeapifyDown(index);
  }
}

void TPpriorityQueue::RemoveAt(int index)
{
  vertices_[index]->SetGain(GetMax()->GetGain() + 1.0);
  // Shift the element to top of the heap
  HeapifyUp(index);
  // Extract the element from the heap
  ExtractMax();
}

void TPtwoWayFM::Refine(const HGraph hgraph,
                        const matrix<float>& max_block_balance,
                        TP_partition& solution)
{
  if (hgraph->num_vertices_ < 500) {
    SetMaxMoves(hgraph->num_vertices_);
  } else {
    SetMaxMoves(50);
  }
  matrix<float> max_block_balance_tol = max_block_balance;
  std::vector<float> paths_cost(hgraph->num_timing_paths_, 0.0);
  GeneratePathsAsEdges(hgraph);
  for (int i = 0; i < refiner_iters_; ++i) {
    if (i == 1) {
      SetTolerance(0.25);
      for (int j = 0; j < num_parts_; ++j) {
        MultiplyFactor(max_block_balance_tol[j], 1.0 + GetTolerance());
      }
    } else {
      SetTolerance(0.0);
      max_block_balance_tol = max_block_balance;
    }
    InitPaths(hgraph, paths_cost, solution);
    Pass(hgraph, max_block_balance, solution, paths_cost);
    if (hgraph->num_timing_paths_ > 0) {
      hgraph->hyperedge_weights_ = hgraph->nonscaled_hyperedge_weights_;
    }
  }
}

std::shared_ptr<VertexGain> TPtwoWayFM::FindMovableVertex(
    const int corking_part,
    const HGraph hgraph,
    TP_gain_buckets& buckets,
    const matrix<float>& curr_block_balance,
    const matrix<float>& max_block_balance)
{
  int parent = 0;
  int total_corking_passes = 25;
  int pass = 0;
  std::shared_ptr<VertexGain> ele_left(new VertexGain);
  std::shared_ptr<VertexGain> ele_right(new VertexGain);
  std::shared_ptr<VertexGain> dummy_cell(new VertexGain);
  int total_elements = buckets[corking_part]->GetTotalElements();
  if (total_elements == 0) {
    return dummy_cell;
  }
  while (true) {
    ++pass;
    int left_child = parent * 2 + 1;
    int right_child = parent * 2 + 2;
    bool left = false;
    bool right = false;
    // If left child is within bounds of PQ
    if (left_child < total_elements) {
      left = true;
      ele_left = buckets[corking_part]->GetHeapVertex(left_child);
    }
    // If right child is within bounds of PQ
    if (right_child < total_elements) {
      right = true;
      ele_right = buckets[corking_part]->GetHeapVertex(right_child);
    }
    // If both children are within bounds of PQ
    if (left == true && right == true) {
      bool legality_of_move_left = CheckLegality(hgraph,
                                                 corking_part,
                                                 ele_left,
                                                 curr_block_balance,
                                                 max_block_balance);
      bool legality_of_move_right = CheckLegality(hgraph,
                                                  corking_part,
                                                  ele_right,
                                                  curr_block_balance,
                                                  max_block_balance);
      // If both moves of children are legal
      if (legality_of_move_left == true && legality_of_move_right == true) {
        // If gain of left child is more than gain of right child
        if (ele_left->GetGain() > ele_right->GetGain()) {
          return ele_left;
        } else {
          return ele_right;
        }
      }  // If move of left child is legal and right child is not legal
      else if (legality_of_move_left == true
               && legality_of_move_right == false) {
        return ele_left;
      }  // If move of right child is legal and left child is not legal
      else if (legality_of_move_left == false
               && legality_of_move_right == true) {
        return ele_right;
      }  // If moves of both children are illegal
      else {
        // Pick the child with the least weight as the next parent
        if (hgraph->vertex_weights_[ele_left->GetVertex()]
            < hgraph->vertex_weights_[ele_right->GetVertex()]) {
          parent = left_child;
        } else {
          parent = right_child;
        }
      }
    }  // If left child is within bounds of PQ and right child is not
    else if (left == true && right == false) {
      bool legality_of_move = CheckLegality(hgraph,
                                            corking_part,
                                            ele_left,
                                            curr_block_balance,
                                            max_block_balance);
      // Check if the movement of the left child is legal
      if (legality_of_move == true) {
        return ele_left;
      } else {
        parent = left_child;
      }
    }  // If right child is within bounds of PQ and left child is not
    else if (left == false && right == true) {
      bool legality_of_move = CheckLegality(hgraph,
                                            corking_part,
                                            ele_right,
                                            curr_block_balance,
                                            max_block_balance);
      // Check if the movement of the left child is legal
      if (legality_of_move == true) {
        return ele_right;
      } else {
        parent = right_child;
      }
    }  // If both children are outside bounds of PQ
    else {
      // return dummy_cell;
      break;
    }
    // If corking passes exceeds threshold, give up
    if (pass > total_corking_passes) {
      break;
    }
  }

  // If still no movable vertex is found, try moving vertices from the other
  // partition

  int flip_corking_part = corking_part == 0 ? 1 : 0;
  int redo_moves = std::min(10, buckets[flip_corking_part]->GetTotalElements());
  for (int i = 0; i < redo_moves; ++i) {
    auto ele = buckets[flip_corking_part]->GetHeapVertex(i);
    if (CheckLegality(hgraph,
                      flip_corking_part,
                      ele,
                      curr_block_balance,
                      max_block_balance)
        == true) {
      return ele;
    }
  }

  return dummy_cell;
}

inline std::shared_ptr<VertexGain> TPtwoWayFM::SolveCorkingEffect(
    const int corking_part,
    const HGraph hgraph,
    TP_gain_buckets& buckets,
    const matrix<float>& curr_block_balance,
    const matrix<float>& max_block_balance)
{
  // Corking happens in part 0 or part 1 is not accessible
  if (corking_part > -1) {
    return FindMovableVertex(
        corking_part, hgraph, buckets, curr_block_balance, max_block_balance);
  }  // Corking happens in both part 0 and 1 and both are accessible
  else {
    if (buckets[0]->GetMax()->GetVertex() < buckets[1]->GetMax()->GetVertex()) {
      return FindMovableVertex(
          0, hgraph, buckets, curr_block_balance, max_block_balance);
    } else {
      return FindMovableVertex(
          1, hgraph, buckets, curr_block_balance, max_block_balance);
    }
  }
}

std::shared_ptr<VertexGain> TPtwoWayFM::PickMoveTwoWay(
    const HGraph hgraph,
    TP_gain_buckets& buckets,
    const matrix<float>& curr_block_balance,
    const matrix<float>& max_block_balance)
{
  int corking_part = -1;
  std::shared_ptr<VertexGain> dummy_cell(new VertexGain);
  if (buckets[0]->GetStatus() == true && buckets[1]->GetStatus() == false) {
    auto ele = buckets[0]->GetMax();
    bool legality_of_move
        = CheckLegality(hgraph, 0, ele, curr_block_balance, max_block_balance);
    if (legality_of_move == true) {
      return ele;
    } else {
      corking_part = 0;
      return SolveCorkingEffect(
          corking_part, hgraph, buckets, curr_block_balance, max_block_balance);
    }
  } else if (buckets[1]->GetStatus() == true
             && buckets[0]->GetStatus() == false) {
    auto ele = buckets[1]->GetMax();
    bool legality_of_move
        = CheckLegality(hgraph, 1, ele, curr_block_balance, max_block_balance);
    if (legality_of_move == true) {
      return ele;
    } else {
      corking_part = 1;
      return SolveCorkingEffect(
          corking_part, hgraph, buckets, curr_block_balance, max_block_balance);
    }
  } else if (buckets[0]->GetStatus() == true
             && buckets[1]->GetStatus() == true) {
    auto ele_0 = buckets[0]->GetMax();
    bool legality_of_move_0 = CheckLegality(
        hgraph, 0, ele_0, curr_block_balance, max_block_balance);
    auto ele_1 = buckets[1]->GetMax();
    bool legality_of_move_1 = CheckLegality(
        hgraph, 1, ele_1, curr_block_balance, max_block_balance);
    if (legality_of_move_0 == true && legality_of_move_1 == true) {
      if (ele_0->GetGain() > ele_1->GetGain()) {
        return ele_0;
      } else {
        return ele_1;
      }
    } else if (legality_of_move_0 == true && legality_of_move_1 == false) {
      return ele_0;
    } else if (legality_of_move_0 == false && legality_of_move_1 == true) {
      return ele_1;
    } else {
      corking_part = -1;
      return SolveCorkingEffect(
          corking_part, hgraph, buckets, curr_block_balance, max_block_balance);
    }
  } else {
    return dummy_cell;
  }
}

void TPtwoWayFM::UpdateNeighbors(const HGraph hgraph,
                                 const std::pair<int, int>& partition_pair,
                                 const std::set<int>& neighbors,
                                 const TP_partition& solution,
                                 const std::vector<float>& cur_path_cost,
                                 const matrix<int>& net_degs,
                                 TP_gain_buckets& gain_buckets)
{
  int from_part = partition_pair.first;
  int to_part = partition_pair.second;
  std::set<int> neighboring_hyperedges;
  // After moving a vertex, loop through all neighbors of that vertex
  for (const int& v : neighbors) {
    int nbr_part = solution[v];
    if (nbr_part != partition_pair.first && nbr_part != partition_pair.second) {
      continue;
    }
    if (GetVisitStatus(v) == true) {
      continue;
    }
    if (hgraph->num_timing_paths_ > 0) {
      const int first_valid_entry = hgraph->vptr_[v];
      const int first_invalid_entry = hgraph->vptr_[v + 1];
      for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
        int he = hgraph->vind_[i];
        neighboring_hyperedges.insert(he);
      }
    }
    int nbr_from_part = nbr_part == from_part ? from_part : to_part;
    int nbr_to_part = nbr_from_part == from_part ? to_part : from_part;
    // If the vertex is in the gain bucket then simply update the priority
    if (gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == true) {
      auto new_gain_cell = CalculateGain(v,
                                         nbr_from_part,
                                         nbr_to_part,
                                         hgraph,
                                         solution,
                                         cur_path_cost,
                                         net_degs);
      float new_gain = new_gain_cell->GetGain();
      // Now update the heap
      int nbr_index_in_heap = gain_buckets[nbr_to_part]->GetLocationOfVertex(v);
      gain_buckets[nbr_to_part]->ChangePriority(nbr_index_in_heap, new_gain);
    }  // If the vertex is a boundary vertex and is not in the gain bucket
       // then add it to the bucket
    else if (CheckBoundaryVertex(hgraph, v, partition_pair, net_degs) == true
             && gain_buckets[nbr_to_part]->CheckIfVertexExists(v) == false) {
      MarkBoundary(v);
      auto gain_cell = CalculateGain(v,
                                     nbr_from_part,
                                     nbr_to_part,
                                     hgraph,
                                     solution,
                                     cur_path_cost,
                                     net_degs);
      gain_buckets[nbr_to_part]->InsertIntoPQ(gain_cell);
    }
  }
}

void TPtwoWayFM::RollbackMoves(std::vector<VertexGain>& trace,
                               matrix<int>& net_degs,
                               int& best_move,
                               float& total_delta_gain,
                               HGraph hgraph,
                               matrix<float>& curr_block_balance,
                               std::vector<float>& cur_path_cost,
                               std::vector<int>& solution)
{
  if (trace.size() == 0) {
    return;
  }
  if (best_move == trace.size() - 1) {
    return;
  }
  int idx = trace.size() - 1;
  while (true) {
    // Grab vertex cell from back of the move trace
    auto vertex_cell = trace.back();
    const int vertex = vertex_cell.GetVertex();
    const int source_part = solution[vertex];
    const int dest_part = source_part == 0 ? 1 : 0;
    const float gain = vertex_cell.GetGain();
    // Deduct gain from tot_delta_gain
    total_delta_gain -= gain;
    // Flip the partition of the vertex to its previous part id
    solution[vertex] = dest_part;
    // Update the balance
    curr_block_balance[dest_part]
        = curr_block_balance[dest_part] + hgraph->vertex_weights_[vertex];
    curr_block_balance[source_part]
        = curr_block_balance[source_part] - hgraph->vertex_weights_[vertex];
    // Update the net degs and collect neighbors
    std::set<int> neighbors;
    const int first_valid_entry_he = hgraph->vptr_[vertex];
    const int first_invalid_entry_he = hgraph->vptr_[vertex + 1];
    for (int i = first_valid_entry_he; i < first_invalid_entry_he; ++i) {
      const int he = hgraph->vind_[i];
      // Updating the net degs here
      ++net_degs[he][dest_part];
      --net_degs[he][source_part];
      // Looping to collect neighbors
    }
    trace.pop_back();
    --idx;
    // Update the trace
    if (idx == best_move) {
      break;
    }
  }
}

void TPtwoWayFM::AcceptTwoWayMove(std::shared_ptr<VertexGain> gain_cell,
                                  HGraph hgraph,
                                  std::vector<VertexGain>& moves_trace,
                                  float& total_gain,
                                  float& total_delta_gain,
                                  std::pair<int, int>& partition_pair,
                                  std::vector<int>& solution,
                                  std::vector<float>& paths_cost,
                                  matrix<float>& curr_block_balance,
                                  TP_gain_buckets& gain_buckets,
                                  matrix<int>& net_degs)
{
  int vertex_id = gain_cell->GetVertex();
  // Push the vertex into the moves_trace
  moves_trace.push_back(*gain_cell);
  // Add gain of the candidate vertex to the total gain
  total_gain -= gain_cell->GetGain();
  total_delta_gain += gain_cell->GetGain();
  // Deactivate given vertex, meaning it will be never be moved again
  MarkVisited(vertex_id);
  gain_cell->SetDeactive();
  // Update the path cost first
  for (int i = 0; i < gain_cell->GetTotalPaths(); ++i) {
    paths_cost[i] = gain_cell->GetPathCost(i);
  }
  // Fetch old and new partitions
  const int prev_part_id = partition_pair.first;
  const int new_part_id = partition_pair.second;
  // Update the partition balance
  curr_block_balance[prev_part_id]
      = curr_block_balance[prev_part_id] - hgraph->vertex_weights_[vertex_id];
  curr_block_balance[new_part_id]
      = curr_block_balance[new_part_id] + hgraph->vertex_weights_[vertex_id];
  // Update the partition
  solution[vertex_id] = new_part_id;
  // update net_degs
  const int first_valid_entry = hgraph->vptr_[vertex_id];
  const int first_invalid_entry = hgraph->vptr_[vertex_id + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    --net_degs[he][prev_part_id];
    ++net_degs[he][new_part_id];
  }
  // Remove vertex from the gain bucket
  int heap_loc = gain_buckets[new_part_id]->GetLocationOfVertex(vertex_id);
  gain_buckets[new_part_id]->RemoveAt(heap_loc);
  if (gain_buckets[new_part_id]->GetSizeOfPQ() == 0) {
    gain_buckets[new_part_id]->SetDeactive();
  }
}

float TPtwoWayFM::Pass(const HGraph hgraph,
                       const matrix<float>& max_block_balance,
                       TP_partition& solution,
                       std::vector<float>& paths_cost)
{
  /*std::vector<float> paths_cost;
  paths_cost.resize(hgraph->num_timing_paths_);*/
  // std::vector<int> path_cuts;
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    paths_cost[path_id] = CalculatePathCost(path_id, hgraph, solution);
  }
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  // METIS related
  /*std::vector<float> avg_block_balance_x
      = DivideFactor(block_balance[0] + block_balance[1], 20);
  std::vector<float> avg_block_balance_y = MultiplyFactor(
      DivideFactor(block_balance[0] + block_balance[1], hgraph->num_vertices_),
      2.0);
  std::vector<float> avg_block_balance = avg_block_balance_x;
  for (int i = 0; i < avg_block_balance.size(); ++i) {
    avg_block_balance[i]
        = std::min(avg_block_balance_x[i], avg_block_balance_y[i]);
  }
  std::vector<float> orig_diff(hgraph->vertex_dimensions_, 0.0);
  for (int i = 0; i < orig_diff.size(); ++i) {
    orig_diff[i] = std::abs(block_balance[0][i] - block_balance[1][i]);
  }*/
  int limit = std::min(
      std::max(static_cast<int>(0.01 * hgraph->num_vertices_), 15), 100);
  matrix<float> max_block_balance_tol = max_block_balance;
  SetTolerance(0.25);
  for (int j = 0; j < num_parts_; ++j) {
    MultiplyFactor(max_block_balance_tol[j], 1.0 + GetTolerance());
  }
  matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  TP_gain_buckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    TP_gain_bucket bucket
        = std::make_shared<TPpriorityQueue>(hgraph->num_vertices_, hgraph);
    buckets.push_back(bucket);
  }
  // Initialize boundary flag
  InitBoundaryFlags(hgraph->num_vertices_);
  // Initialize the visit flags to false meaning no vertex has been visited
  InitVisitFlags(hgraph->num_vertices_);
  auto partition_pair = std::make_pair(0, 1);
  std::vector<int> boundary_vertices
      = FindBoundaryVertices(hgraph, net_degs, partition_pair);
  InitGainBucketsTwoWay(
      hgraph, solution, net_degs, boundary_vertices, paths_cost, buckets);
  // Moves begin
  std::vector<VertexGain> moves_trace;
  float cutsize = CutEvaluator(hgraph, solution).first;
  float min_cut = cutsize;
  float total_delta_gain = 0.0;
  int best_move = -1;
  std::vector<int> pre_fm = solution;
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    auto ele
        = PickMoveTwoWay(hgraph, buckets, block_balance, max_block_balance);
    if (ele->GetStatus() == false) {
      break;
    }
    int vertex = ele->GetVertex();
    int from_part = solution[vertex];
    int to_part = from_part == 0 ? 1 : 0;
    std::pair<int, int> partition_pair = std::make_pair(from_part, to_part);
    AcceptTwoWayMove(ele,
                     hgraph,
                     moves_trace,
                     cutsize,
                     total_delta_gain,
                     partition_pair,
                     solution,
                     paths_cost,
                     block_balance,
                     buckets,
                     net_degs);
    std::set<int> neighbors;
    FindNeighbors(hgraph, vertex, partition_pair, solution, neighbors, false);
    UpdateNeighboringPaths(
        partition_pair, neighbors, hgraph, solution, paths_cost);
    UpdateNeighbors(hgraph,
                    partition_pair,
                    neighbors,
                    solution,
                    paths_cost,
                    net_degs,
                    buckets);
    if (cutsize < min_cut) {
      min_cut = cutsize;
      best_move = i;
    } else if (i - best_move > limit) {
      break;
    }
  }
  if (best_move == 0) {
    solution = pre_fm;
  } else {
    RollbackMoves(moves_trace,
                  net_degs,
                  best_move,
                  total_delta_gain,
                  hgraph,
                  block_balance,
                  paths_cost,
                  solution);
  }

  /*if (best_move > 0) {
    ReweighHyperedges();
  }*/

  return total_delta_gain;
}

void TPtwoWayFM::InitGainBucketsTwoWay(
    const HGraph hgraph,
    TP_partition& solution,
    const matrix<int> net_degs,
    const std::vector<int>& boundary_vertices,
    const std::vector<float>& cur_path_cost,
    TP_gain_buckets& buckets)
{
  buckets[0]->SetActive();
  buckets[1]->SetActive();
  assert(buckets[0]->GetStatus() == true);
  assert(buckets[1]->GetStatus() == true);
  for (const int& v : boundary_vertices) {
    if (GetBoundaryStatus(v) == false) {
      MarkBoundary(v);
    }
    // Reset visit flag
    if (GetVisitStatus(v) == true) {
      ResetVisited(v);
    }
    const int from_part = solution[v] == 0 ? 0 : 1;
    const int to_part = from_part == 0 ? 1 : 0;
    auto gain_cell = CalculateGain(
        v, from_part, to_part, hgraph, solution, cur_path_cost, net_degs);
    buckets[to_part]->InsertIntoPQ(gain_cell);
  }
  if (buckets[0]->GetTotalElements() == 0) {
    buckets[0]->SetDeactive();
  }
  if (buckets[1]->GetTotalElements() == 0) {
    buckets[1]->SetDeactive();
  }
}

void TPtwoWayFM::BalancePartition(const HGraph hgraph,
                                  const matrix<float>& max_block_balance,
                                  std::vector<int>& solution)
{
  std::vector<float> total_vertex_wts = hgraph->GetTotalVertexWeights();
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  bool early_balance_check = true;
  for (int i = 0; i < num_parts_; ++i) {
    if (block_balance[i] > max_block_balance[i]) {
      early_balance_check = false;
      break;
    }
  }
  if (early_balance_check == true) {
    return;
  }

  int num_visited_vertex = 0;
  std::vector<bool> visited(hgraph->num_vertices_, false);
  if (hgraph->fixed_vertex_flag_ == true) {
    for (auto v = 0; v < hgraph->num_vertices_; ++v) {
      if (hgraph->fixed_attr_[v] > -1) {
        visited[v] = true;
        ++num_visited_vertex;
      }
    }
  }
  std::vector<float> paths_cost(hgraph->num_timing_paths_, 0.0);
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    ReweighTimingWeights(path_id, hgraph, solution, paths_cost);
    // paths_cost.push_back(CalculatePathCost(path_id, hgraph, solution));
  }
  TP_gain_buckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    TP_gain_bucket bucket
        = std::make_shared<TPpriorityQueue>(hgraph->num_vertices_, hgraph);
    buckets.push_back(bucket);
  }
  // Initialize boundary flag
  InitBoundaryFlags(hgraph->num_vertices_);
  // Initialize the visit flags to false meaning no vertex has been visited
  InitVisitFlags(hgraph->num_vertices_);
  std::vector<int> boundary_vertices(hgraph->num_vertices_);
  std::iota(boundary_vertices.begin(), boundary_vertices.end(), 0);
  InitGainBucketsTwoWay(
      hgraph, solution, net_degs, boundary_vertices, paths_cost, buckets);
  std::vector<VertexGain> moves_trace;
  float cutsize = 0.0;
  float total_delta_gain = 0.0;
  matrix<float> min_block_balance;
  min_block_balance.push_back(total_vertex_wts - max_block_balance[0]);
  min_block_balance.push_back(total_vertex_wts - max_block_balance[1]);
  while (true) {
    if (buckets[1]->GetStatus() == false) {
      break;
    }
    auto ele = buckets[1]->GetMax();
    if (ele->GetStatus() == false) {
      break;
    }
    int vertex = ele->GetVertex();
    int from_part = 0;
    int to_part = 1;
    std::pair<int, int> partition_pair = std::make_pair(from_part, to_part);
    AcceptTwoWayMove(ele,
                     hgraph,
                     moves_trace,
                     cutsize,
                     total_delta_gain,
                     partition_pair,
                     solution,
                     paths_cost,
                     block_balance,
                     buckets,
                     net_degs);
    std::set<int> neighbors;
    FindNeighbors(hgraph, vertex, partition_pair, solution, neighbors, false);
    UpdateNeighbors(hgraph,
                    partition_pair,
                    neighbors,
                    solution,
                    paths_cost,
                    net_degs,
                    buckets);
    if (block_balance[1] >= min_block_balance[1]) {
      break;
    }
  }
}

// Flat K-way FM implementation starts here

void TPkWayFM::InitializeSingleGainBucket(
    TP_gain_buckets& buckets,
    int to_pid,
    const std::vector<int>& boundary_vertices,
    const HGraph hgraph,
    const TP_partition& solution,
    const std::vector<float>& cur_path_cost,
    const matrix<int>& net_degs)
{
  buckets[to_pid]->SetActive();
  assert(buckets[to_pid]->GetStatus());
  for (const int& v : boundary_vertices) {
    if (GetBoundaryStatus(v) == false) {
      MarkBoundary(v);
    }
    if (GetVisitStatus(v) == true) {
      ResetVisited(v);
    }
    const int from_part = solution[v];
    auto gain_cell = CalculateGain(
        v, from_part, to_pid, hgraph, solution, cur_path_cost, net_degs);
    buckets[to_pid]->InsertIntoPQ(gain_cell);
  }
  if (buckets[to_pid]->GetTotalElements() == 0) {
    buckets[to_pid]->SetDeactive();
  }
}

void TPkWayFM::InitializeGainBucketsKWay(
    const HGraph hgraph,
    const TP_partition& solution,
    const matrix<int>& net_degs,
    const std::vector<int>& boundary_vertices,
    const std::vector<float>& cur_path_cost,
    TP_gain_buckets& buckets)
{
  std::vector<std::thread> threads;  // for parallel updating
  // parallel initialize the num_parts gain_buckets
  for (int to_pid = 0; to_pid < num_parts_; to_pid++)
    threads.push_back(std::thread(&TPkWayFM::InitializeSingleGainBucket,
                                  this,
                                  std::ref(buckets),
                                  to_pid,
                                  boundary_vertices,
                                  hgraph,
                                  solution,
                                  cur_path_cost,
                                  net_degs));
  for (auto& t : threads)
    t.join();  // wait for all threads to finish
  threads.clear();
}

void TPkWayFM::Refine(const HGraph hgraph,
                      const matrix<float>& max_block_balance,
                      TP_partition& solution)
{
  std::vector<float> paths_cost(hgraph->num_timing_paths_, 0.0);
  GeneratePathsAsEdges(hgraph);
  for (int i = 0; i < refiner_iters_; ++i) {
    InitPaths(hgraph, paths_cost, solution);
    Pass(hgraph, max_block_balance, solution, paths_cost);
    if (hgraph->num_timing_paths_ > 0) {
      hgraph->hyperedge_weights_ = hgraph->nonscaled_hyperedge_weights_;
    }
  }
}

void TPkWayFM::RollbackMovesKWay(std::vector<VertexGain>& trace,
                                 matrix<int>& net_degs,
                                 int& best_move,
                                 float& total_delta_gain,
                                 HGraph hgraph,
                                 matrix<float>& curr_block_balance,
                                 std::vector<float>& cur_path_cost,
                                 std::vector<int>& solution,
                                 std::vector<int>& partition_trace)
{
  if (trace.size() == 0) {
    return;
  }
  if (best_move == trace.size() - 1) {
    return;
  }
  int idx = trace.size() - 1;
  while (true) {
    // Grab vertex cell from back of the move trace
    auto vertex_cell = trace.back();
    const int vertex = vertex_cell.GetVertex();
    const int source_part = solution[vertex];
    const int dest_part = partition_trace.back();
    const float gain = vertex_cell.GetGain();
    // Deduct gain from tot_delta_gain
    total_delta_gain -= gain;
    // Flip the partition of the vertex to its previous part id
    solution[vertex] = dest_part;
    // Update the balance
    curr_block_balance[dest_part]
        = curr_block_balance[dest_part] + hgraph->vertex_weights_[vertex];
    curr_block_balance[source_part]
        = curr_block_balance[source_part] - hgraph->vertex_weights_[vertex];
    // Update the net degs and collect neighbors
    std::set<int> neighbors;
    const int first_valid_entry_he = hgraph->vptr_[vertex];
    const int first_invalid_entry_he = hgraph->vptr_[vertex + 1];
    for (int i = first_valid_entry_he; i < first_invalid_entry_he; ++i) {
      const int he = hgraph->vind_[i];
      // Updating the net degs here
      ++net_degs[he][dest_part];
      --net_degs[he][source_part];
      // Looping to collect neighbors
    }
    trace.pop_back();
    partition_trace.pop_back();
    --idx;
    // Update the trace
    if (idx == best_move) {
      break;
    }
  }
}

void TPkWayFM::UpdateSingleGainBucket(int part,
                                      const std::set<int>& neighbors,
                                      TP_gain_buckets& buckets,
                                      const HGraph hgraph,
                                      const TP_partition& solution,
                                      const std::vector<float>& cur_path_cost,
                                      const matrix<int>& net_degs)
{
  std::set<int> neighboring_hyperedges;
  for (const int& v : neighbors) {
    int from_part = solution[v];
    if (from_part == part) {
      continue;
    }
    if (GetVisitStatus(v) == true) {
      continue;
    }
    /*if (hgraph->num_timing_paths_ > 0) {
      const int first_valid_entry = hgraph->vptr_[v];
      const int first_invalid_entry = hgraph->vptr_[v + 1];
      for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
        int he = hgraph->vind_[i];
        neighboring_hyperedges.push_back(he);
      }
    }*/
    std::pair<int, int> partition_pair = std::make_pair(from_part, part);
    if (buckets[part]->CheckIfVertexExists(v) == true) {
      auto new_gain_cell = CalculateGain(
          v, from_part, part, hgraph, solution, cur_path_cost, net_degs);
      float new_gain = new_gain_cell->GetGain();
      int nbr_index_in_heap = buckets[part]->GetLocationOfVertex(v);
      buckets[part]->ChangePriority(nbr_index_in_heap, new_gain);
    } else if (CheckBoundaryVertex(hgraph, v, partition_pair, net_degs) == true
               && buckets[part]->CheckIfVertexExists(v) == false) {
      MarkBoundary(v);
      auto gain_cell = CalculateGain(
          v, from_part, part, hgraph, solution, cur_path_cost, net_degs);
      buckets[part]->InsertIntoPQ(gain_cell);
    }
  }
}

std::shared_ptr<VertexGain> TPkWayFM::PickMoveKWay(
    const HGraph hgraph,
    TP_gain_buckets& buckets,
    const matrix<float>& curr_block_balance,
    const matrix<float>& max_block_balance)
{
  int to_pid = -1;
  std::shared_ptr<VertexGain> candidate = std::make_shared<VertexGain>(
      -1, -std::numeric_limits<float>::max());  // best_candidate
  // best gain bucket for "corking effect"
  int best_to_pid = -1;  // block id with best_gain
  float best_gain = -std::numeric_limits<float>::max();
  std::shared_ptr<VertexGain> dummy_cell(new VertexGain);
  for (int i = 0; i < num_parts_; ++i) {
    if (buckets[i]->GetStatus() == false) {
      continue;
    }
    auto ele = buckets[i]->GetMax();
    int vertex = ele->GetVertex();
    float gain = ele->GetGain();
    if ((gain > candidate->GetGain())
        && (curr_block_balance[i] + hgraph->vertex_weights_[vertex]
            < max_block_balance[i])) {
      to_pid = i;
      candidate = ele;
    }
    // record part for solving corking effect
    if (gain > best_gain) {
      best_gain = gain;
      best_to_pid = i;
    }
  }

  if (to_pid > -1) {
    candidate->SetPotentialMove(to_pid);
    return candidate;
  }

  // "corking effect", i.e., no candidate
  int total_elements = buckets.at(best_to_pid)->GetTotalElements();
  if (total_elements == 0) {
    return dummy_cell;
  }
  std::shared_ptr<VertexGain> ele_left(new VertexGain);
  std::shared_ptr<VertexGain> ele_right(new VertexGain);

  if (to_pid == -1) {
    int pass = 0;
    int parent = 0;
    int total_corking_passes = 25;
    while (true) {
      ++pass;
      int left_child = parent * 2 + 1;
      int right_child = parent * 2 + 2;
      bool left = false;
      bool right = false;
      // If left child is within bounds of PQ
      if (left_child < total_elements) {
        left = true;
        ele_left = buckets[best_to_pid]->GetHeapVertex(left_child);
      }
      if (right_child < total_elements) {
        right = true;
        ele_right = buckets[best_to_pid]->GetHeapVertex(right_child);
      }
      if (left == true && right == true) {
        bool legality_of_move_left = CheckLegality(hgraph,
                                                   best_to_pid,
                                                   ele_left,
                                                   curr_block_balance,
                                                   max_block_balance);
        bool legality_of_move_right = CheckLegality(hgraph,
                                                    best_to_pid,
                                                    ele_right,
                                                    curr_block_balance,
                                                    max_block_balance);
        if (legality_of_move_left == true && legality_of_move_right == true) {
          if (ele_left->GetGain() > ele_right->GetGain()) {
            ele_left->SetPotentialMove(best_to_pid);
            return ele_left;
          } else {
            ele_right->SetPotentialMove(best_to_pid);
            return ele_right;
          }
        } else if (legality_of_move_left == true
                   && legality_of_move_right == false) {
          ele_left->SetPotentialMove(best_to_pid);
          return ele_left;
        } else if (legality_of_move_left == false
                   && legality_of_move_right == true) {
          ele_right->SetPotentialMove(best_to_pid);
          return ele_right;
        } else {
          if (hgraph->vertex_weights_[ele_left->GetVertex()]
              < hgraph->vertex_weights_[ele_right->GetVertex()]) {
            parent = left_child;
          } else {
            parent = right_child;
          }
        }
      } else if (left == true && right == false) {
        bool legality_of_move = CheckLegality(hgraph,
                                              best_to_pid,
                                              ele_left,
                                              curr_block_balance,
                                              max_block_balance);
        if (legality_of_move == true) {
          ele_left->SetPotentialMove(best_to_pid);
          return ele_left;
        } else {
          parent = left_child;
        }
      } else if (left == false && right == true) {
        bool legality_of_move = CheckLegality(hgraph,
                                              best_to_pid,
                                              ele_right,
                                              curr_block_balance,
                                              max_block_balance);
        if (legality_of_move == true) {
          ele_right->SetPotentialMove(best_to_pid);
          return ele_right;
        } else {
          parent = right_child;
        }
      } else {
        return dummy_cell;
      }
      if (pass > total_corking_passes) {
        return dummy_cell;
      }
    }
  }
  logger_->error(utl::PAR, 76, "No best move found in PickMoveKWay");
}

// Remove vertex from a heap
void TPkWayFM::HeapEleDeletion(int vertex_id,
                               int part,
                               TP_gain_buckets& buckets)
{
  if (buckets[part]->CheckIfVertexExists(vertex_id) == false) {
    return;
  }
  int heap_loc = buckets[part]->GetLocationOfVertex(vertex_id);
  buckets[part]->RemoveAt(heap_loc);
  if (buckets[part]->GetSizeOfPQ() == 0) {
    buckets[part]->SetDeactive();
  }
}

void TPkWayFM::AcceptKWayMove(std::shared_ptr<VertexGain> gain_cell,
                              HGraph hgraph,
                              std::vector<VertexGain>& moves_trace,
                              float& total_gain,
                              float& total_delta_gain,
                              std::pair<int, int>& partition_pair,
                              std::vector<int>& solution,
                              std::vector<float>& paths_cost,
                              matrix<float>& curr_block_balance,
                              TP_gain_buckets& gain_buckets,
                              matrix<int>& net_degs)
{
  int vertex_id = gain_cell->GetVertex();
  // Push the vertex into the moves_trace
  moves_trace.push_back(*gain_cell);
  // Add gain of the candidate vertex to the total gain
  total_gain -= gain_cell->GetGain();
  total_delta_gain += gain_cell->GetGain();
  // Deactivate given vertex, meaning it will be never be moved again
  MarkVisited(vertex_id);
  gain_cell->SetDeactive();
  // Update the path cost first
  for (int i = 0; i < gain_cell->GetTotalPaths(); ++i) {
    paths_cost[i] = gain_cell->GetPathCost(i);
  }
  // Fetch old and new partitions
  const int prev_part_id = partition_pair.first;
  const int new_part_id = partition_pair.second;
  // Update the partition balance
  curr_block_balance[prev_part_id]
      = curr_block_balance[prev_part_id] - hgraph->vertex_weights_[vertex_id];
  curr_block_balance[new_part_id]
      = curr_block_balance[new_part_id] + hgraph->vertex_weights_[vertex_id];
  // Update the partition
  solution[vertex_id] = new_part_id;
  // update net_degs
  const int first_valid_entry = hgraph->vptr_[vertex_id];
  const int first_invalid_entry = hgraph->vptr_[vertex_id + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i];
    --net_degs[he][prev_part_id];
    ++net_degs[he][new_part_id];
  }
  // Remove vertex from all buckets where vertex is present
  std::vector<std::thread> deletion_threads;
  for (int i = 0; i < num_parts_; ++i) {
    deletion_threads.push_back(std::thread(&par::TPkWayFM::HeapEleDeletion,
                                           this,
                                           vertex_id,
                                           i,
                                           std::ref(gain_buckets)));
  }
  for (auto& th : deletion_threads) {
    th.join();
  }
  /*int heap_loc = gain_buckets[new_part_id]->GetLocationOfVertex(vertex_id);
  gain_buckets[new_part_id]->RemoveAt(heap_loc);
  if (gain_buckets[new_part_id]->GetSizeOfPQ() == 0) {
    gain_buckets[new_part_id]->SetDeactive();
  }*/
}

// Comment from Zhiang : No Magical Numbers !!!
// To do list (20230204):
// (1) define variables for these literal numbers
// (2) make these variables tunable, so put them into
//     the class member variables
float TPkWayFM::Pass(const HGraph hgraph,
                     const matrix<float>& max_block_balance,
                     TP_partition& solution,
                     std::vector<float>& paths_cost)
{
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  // XXX Please rewrite this
  int limit = std::min(
      std::max(static_cast<int>(0.01 * hgraph->num_vertices_), 15), 100);
  matrix<float> max_block_balance_tol = max_block_balance;
  // XXX Please rewrite this
  SetTolerance(0.25);
  for (int j = 0; j < num_parts_; ++j) {
    MultiplyFactor(max_block_balance_tol[j], 1.0 + GetTolerance());
  }
  matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  TP_gain_buckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    TP_gain_bucket bucket
        = std::make_shared<TPpriorityQueue>(hgraph->num_vertices_, hgraph);
    buckets.push_back(bucket);
  }
  // Initialize boundary flag
  InitBoundaryFlags(hgraph->num_vertices_);
  // Initialize the visit flags to false meaning no vertex has been visited
  InitVisitFlags(hgraph->num_vertices_);
  // XXX Please add the comments to explain the meaning of 0 and 1
  auto partition_pair = std::make_pair(0, 1);
  std::vector<int> boundary_vertices
      = FindBoundaryVertices(hgraph, net_degs, partition_pair);

  // Initialize current gain in a multi-thread manner
  // set based on max heap (k set)
  // each block has its own max heap

  InitializeGainBucketsKWay(
      hgraph, solution, net_degs, boundary_vertices, paths_cost, buckets);

  VertexGain global_best_ver_gain(-1, -std::numeric_limits<float>::max());
  std::vector<int> pre_fm = solution;
  std::vector<int> move_trace;  // store the moved vertices in sequence
  std::vector<int> partition_trace;
  std::vector<VertexGain> moves_trace;
  float cutsize = CutEvaluator(hgraph, solution).first;
  float min_cut = cutsize;
  float total_delta_gain = 0.0;
  int best_move = -1;
  // Main loop of FM pass
  for (int i = 0; i < GetMaxMoves(); ++i) {
    auto candidate
        = PickMoveKWay(hgraph, buckets, block_balance, max_block_balance);
    if (candidate->GetStatus() == false) {
      break;
    }
    // update the state of the partitioning
    int vertex = candidate->GetVertex();  // candidate vertex
    int from_part = solution[vertex];
    int to_part = candidate->GetPotentialMove();
    partition_trace.push_back(from_part);
    std::pair<int, int> partition_pair = std::make_pair(from_part, to_part);
    AcceptKWayMove(candidate,
                   hgraph,
                   moves_trace,
                   cutsize,
                   total_delta_gain,
                   partition_pair,
                   solution,
                   paths_cost,
                   block_balance,
                   buckets,
                   net_degs);
    std::set<int> neighbors;
    FindNeighbors(hgraph, vertex, partition_pair, solution, neighbors, true);
    UpdateNeighboringPaths(
        partition_pair, neighbors, hgraph, solution, paths_cost);
    // update the neighbors of v for all gain buckets in parallel
    std::vector<std::thread> threads;
    for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
      threads.push_back(std::thread(&TPkWayFM::UpdateSingleGainBucket,
                                    this,
                                    to_pid,
                                    neighbors,
                                    std::ref(buckets),
                                    hgraph,
                                    solution,
                                    paths_cost,
                                    net_degs));
    }
    for (auto& t : threads)
      t.join();  // wait for all threads to finish
    threads.clear();

    if (cutsize < min_cut) {
      min_cut = cutsize;
      best_move = i;
    } else if (i - best_move > limit) {
      break;
    }
    /*timing_cuts = GetTimingCuts(hgraph, solution);
    if (hgraph->num_timing_paths_ > 0) {
      int total_critical_paths_cut = timing_cuts.first;
      int worst_cut = timing_cuts.second;
      int delta_critical_cut
          = best_total_critical_paths_cut - total_critical_paths_cut;
      int delta_worst_cut = best_critical_cut - worst_cut;
      int delta_cut = min_cut - cutsize;
      float overall_delta
          = critical_factor * static_cast<float>(delta_critical_cut)
            + worst_factor * static_cast<float>(delta_worst_cut)
            + cutsize_factor * static_cast<float>(delta_cut);
      if (total_critical_paths_cut < best_total_critical_paths_cut) {
        best_total_critical_paths_cut = total_critical_paths_cut;
      }
      if (worst_cut < best_critical_cut) {
        best_critical_cut = worst_cut;
      }
      if (cutsize < min_cut) {
        min_cut = cutsize;
      }
      if (overall_delta > 0) {
        best_move = i;
      } else if (i - best_move > limit) {
        break;
      }
    } else {
      if (cutsize < min_cut) {
        min_cut = cutsize;
        best_move = i;
      } else if (i - best_move > limit) {
        break;
      }
    }*/
  }
  if (best_move <= 0) {
    solution = pre_fm;
  } else {
    RollbackMovesKWay(moves_trace,
                      net_degs,
                      best_move,
                      total_delta_gain,
                      hgraph,
                      block_balance,
                      paths_cost,
                      solution,
                      partition_trace);
  }

  return total_delta_gain;
}

// Greedy two way refinement implementation starts here

void TPgreedyRefine::Refine(const HGraph hgraph,
                            const matrix<float>& max_block_balance,
                            TP_partition& solution)
{
  for (int i = 0; i < refiner_iters_; ++i) {
    Pass(hgraph, solution, max_block_balance);
  }
}

float TPgreedyRefine::CalculateGain(HGraph hgraph,
                                    int straddle_he,
                                    int from,
                                    int to,
                                    std::vector<int>& straddle,
                                    matrix<int>& net_degs)
{
  float gain
      = std::inner_product(hgraph->hyperedge_weights_[straddle_he].begin(),
                           hgraph->hyperedge_weights_[straddle_he].end(),
                           e_wt_factors_.begin(),
                           0.0);
  for (auto& v : straddle) {
    const int first_valid_entry = hgraph->vptr_[v];
    const int first_invalid_entry = hgraph->vptr_[v + 1];
    for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
      const int he = hgraph->vind_[i];
      auto pre_net_deg = net_degs[he];
      auto post_net_deg = pre_net_deg;
      --post_net_deg[from];
      ++post_net_deg[to];
      // If the hyperedge was straddling the cut but post movement does not
      // then this is a gain
      if (pre_net_deg[from] > 0 && pre_net_deg[to] > 0) {
        if (post_net_deg[from] == 0 && post_net_deg[to] > 0) {
          gain += std::inner_product(hgraph->hyperedge_weights_[he].begin(),
                                     hgraph->hyperedge_weights_[he].end(),
                                     e_wt_factors_.begin(),
                                     0.0);
        }
      }
      // If the hyperedge was not straddling the cut but post movement it
      // straddles the cut then this is a negative gain
      if (pre_net_deg[from] > 0 && pre_net_deg[to] == 0) {
        if (post_net_deg[from] > 0 && post_net_deg[to] > 0) {
          gain -= std::inner_product(hgraph->hyperedge_weights_[he].begin(),
                                     hgraph->hyperedge_weights_[he].end(),
                                     e_wt_factors_.begin(),
                                     0.0);
        }
      }
    }
  }
  return gain;
}

void TPgreedyRefine::CommitStraddle(int to,
                                    HGraph hgraph,
                                    std::vector<int>& straddle,
                                    TP_partition& solution,
                                    matrix<int>& net_degs,
                                    matrix<float>& block_balance)
{
  int from = to == 0 ? 1 : 0;
  for (auto& v : straddle) {
    solution[v] = to;
    const int first_valid_entry = hgraph->vptr_[v];
    const int first_invalid_entry = hgraph->vptr_[v + 1];
    block_balance[from] = block_balance[from] - hgraph->vertex_weights_[v];
    block_balance[to] = block_balance[to] + hgraph->vertex_weights_[v];
    for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
      int he = hgraph->vind_[i];
      --net_degs[he][from];
      ++net_degs[he][to];
    }
  }
}

float TPgreedyRefine::MoveStraddle(int he,
                                   std::vector<int>& straddle_0,
                                   std::vector<int>& straddle_1,
                                   HGraph hgraph,
                                   TP_partition& solution,
                                   matrix<int>& net_degs,
                                   matrix<float>& block_balance,
                                   const matrix<float>& max_block_balance)
{
  std::vector<float> wts_straddle_0(hgraph->vertex_dimensions_, 0.0);
  std::vector<float> wts_straddle_1(hgraph->vertex_dimensions_, 0.0);
  for (auto& v : straddle_0) {
    wts_straddle_0 = wts_straddle_0 + hgraph->vertex_weights_[v];
  }
  for (auto& v : straddle_1) {
    wts_straddle_1 = wts_straddle_1 + hgraph->vertex_weights_[v];
  }
  float gain_straddle_0 = -hgraph->num_hyperedges_;
  float gain_straddle_1 = -hgraph->num_hyperedges_;
  if (block_balance[1] + wts_straddle_0 < max_block_balance[1]) {
    gain_straddle_0 = CalculateGain(hgraph, he, 0, 1, straddle_0, net_degs);
  } else if (block_balance[0] + wts_straddle_1 < max_block_balance[0]) {
    gain_straddle_1 = CalculateGain(hgraph, he, 1, 0, straddle_1, net_degs);
  }
  if (gain_straddle_0 >= 0 && gain_straddle_1 < 0) {
    CommitStraddle(1, hgraph, straddle_0, solution, net_degs, block_balance);
    return gain_straddle_0;
  } else if (gain_straddle_0 < 0 && gain_straddle_1 >= 0) {
    CommitStraddle(0, hgraph, straddle_1, solution, net_degs, block_balance);
    return gain_straddle_1;
  } else if (gain_straddle_0 >= 0 && gain_straddle_1 < 0) {
    if (gain_straddle_0 > gain_straddle_1) {
      CommitStraddle(1, hgraph, straddle_0, solution, net_degs, block_balance);
      return gain_straddle_0;
    } else {
      CommitStraddle(0, hgraph, straddle_1, solution, net_degs, block_balance);
      return gain_straddle_1;
    }
  } else {
    return 0.0;
  }
}

float TPgreedyRefine::Pass(HGraph hgraph,
                           TP_partition& solution,
                           const matrix<float>& max_vertex_balance)
{
  std::vector<float> paths_cost;
  paths_cost.resize(hgraph->num_timing_paths_);
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++) {
    paths_cost[path_id] = CalculatePathCost(path_id, hgraph, solution);
  }
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  float total_gain = 0.0;
  int iter = 0;
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    if (net_degs[i][0] > 0 && net_degs[i][1] > 0) {
      ++iter;
      if (iter == max_moves_) {
        return total_gain;
      }
      const int first_valid_entry = hgraph->eptr_[i];
      const int first_invalid_entry = hgraph->eptr_[i + 1];
      std::vector<int> straddled_vertices_0;
      std::vector<int> straddled_vertices_1;
      for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
        int v = hgraph->eind_[j];
        if (solution[v] == 0) {
          straddled_vertices_0.push_back(v);
        } else {
          straddled_vertices_1.push_back(v);
        }
      }
      float gain = MoveStraddle(i,
                                straddled_vertices_0,
                                straddled_vertices_1,
                                hgraph,
                                solution,
                                net_degs,
                                block_balance,
                                max_vertex_balance);
      total_gain += gain;
    }
    if (i == hgraph->num_hyperedges_ - 1) {
      return total_gain;
    }
  }
  logger_->error(utl::PAR, 77, "No gain found in Pass");
}

// Ilp refiner implementation starts here

float TPilpRefine::CalculateGain(int vertex,
                                 TP_partition& solution,
                                 HGraph hgraph,
                                 matrix<int>& net_degs)
{
  float score = 0.0;
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto& num_v : net_degs[e])
      if (num_v > 0)
        connectivity++;
    return connectivity;
  };
  // function : calculate the score for the hyperedge
  auto GetHyperedgeScore = [&](int e) {
    return std::inner_product(hgraph->hyperedge_weights_[e].begin(),
                              hgraph->hyperedge_weights_[e].end(),
                              e_wt_factors_.begin(),
                              0.0);
  };
  // traverse all the hyperedges connected to v
  const int first_valid_entry = hgraph->vptr_[vertex];
  const int first_invalid_entry = hgraph->vptr_[vertex + 1];
  int from_part = solution[vertex];
  int to_part = from_part == 0 ? 1 : 0;
  for (auto e_idx = first_valid_entry; e_idx < first_invalid_entry; ++e_idx) {
    const int e = hgraph->vind_[e_idx];  // hyperedge id
    const int connectivity = GetConnectivity(e);
    const float e_score = GetHyperedgeScore(e);
    const int he_size = hgraph->eptr_[e + 1] - hgraph->eptr_[e];
    if (he_size > thr_he_size_skip_) {
      continue;
    }
    if (connectivity == 1
        && net_degs[e][from_part]
               > 1) {  // move from_pid to to_pid will have negative socre
      score -= e_score;
    } else if (connectivity == 2 && net_degs[e][from_part] == 1
               && net_degs[e][to_part] > 0) {
      score += e_score;  // after move, all the vertices in to_pid
    }
  }
  return score;
}

void TPilpRefine::OrderVertexSet(HGraph hgraph,
                                 std::vector<int>& vertices,
                                 TP_partition& solution,
                                 matrix<int>& net_degs,
                                 matrix<float>& block_balance,
                                 const matrix<float>& max_block_balance)
{
  std::vector<float> gain_vertices(hgraph->num_vertices_, 0.0);
  for (int i = 0; i < vertices.size(); ++i) {
    gain_vertices[i] = CalculateGain(vertices[i], solution, hgraph, net_degs);
  }
  auto gain_comparison = [&](int x, int y) {
    if (gain_vertices[x] > gain_vertices[y]) {
      const int from = solution[x];
      const int to = from == 0 ? 1 : 0;
      if (block_balance[to] + hgraph->vertex_weights_[x]
          < max_block_balance[to]) {
        return true;
      } else {
        return false;
      }
    }
    return false;
  };
  std::sort(vertices.begin(), vertices.end(), gain_comparison);
}

void TPilpRefine::ContractNonBoundary(HGraph hgraph,
                                      std::vector<int>& vertices,
                                      TP_partition& solution)
{
  int cluster_id = 0;
  cluster_map_.resize(hgraph->num_vertices_);
  std::fill(cluster_map_.begin(), cluster_map_.end(), -1);
  for (auto& v : vertices) {
    cluster_map_[v] = cluster_id++;
  }
  if (hgraph->fixed_vertex_flag_ == true) {
    for (int i = 0; i < hgraph->fixed_attr_.size(); ++i) {
      if (hgraph->fixed_attr_[i] > -1) {
        cluster_map_[i] = cluster_id++;
      }
    }
  }
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    if (cluster_map_[i] > -1) {
      continue;
    }
    if (solution[i] == 0) {
      cluster_map_[i] = cluster_id;
    } else {
      cluster_map_[i] = cluster_id + 1;
    }
  }
}

std::shared_ptr<TPilpGraph> TPilpRefine::ContractHypergraph(
    HGraph hgraph,
    TP_partition& solution,
    int wavefront)
{
  int total_ilp_vtxs
      = *std::max_element(cluster_map_.begin(), cluster_map_.end()) + 1;
  matrix<float> vtx_wts_crs(
      total_ilp_vtxs, std::vector<float>(hgraph->vertex_dimensions_, 0.0));
  std::vector<int> fixed_vtxs_crs(total_ilp_vtxs, -1);
  std::fill(vtx_wts_crs.begin(),
            vtx_wts_crs.end(),
            std::vector<float>(hgraph->vertex_dimensions_, 0.0));
  for (int i = 0; i < cluster_map_.size(); ++i) {
    const int cid = cluster_map_[i];
    vtx_wts_crs[cid] = vtx_wts_crs[cid] + hgraph->vertex_weights_[i];
    if (cid >= wavefront) {
      fixed_vtxs_crs[cid] = solution[i];
    } else {
      fixed_vtxs_crs[cid] = -1;
    }
  }
  matrix<int> hes_crs;
  matrix<float> hes_wts_crs;
  std::map<long long int, int> hash_map;
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    const int first_valid_entry = hgraph->eptr_[i];
    const int first_invalid_entry = hgraph->eptr_[i + 1];
    const int he_size = first_invalid_entry - first_valid_entry;
    if (he_size <= 1) {
      continue;
    }
    std::set<int> he_crs;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int vtx = hgraph->eind_[j];
      he_crs.insert(cluster_map_[vtx]);
    }
    if (he_crs.size() <= 1) {
      continue;
    }
    const long long int hash_value
        = std::inner_product(he_crs.begin(),
                             he_crs.end(),
                             he_crs.begin(),
                             static_cast<long long int>(0));
    if (hash_map.find(hash_value) == hash_map.end()) {
      hash_map[hash_value] = static_cast<int>(hes_crs.size());
      hes_wts_crs.push_back(hgraph->hyperedge_weights_[i]);
      hes_crs.push_back(std::vector<int>(he_crs.begin(), he_crs.end()));
    } else {
      const int hash_id = hash_map[hash_value];
      std::vector<int> he_vec(he_crs.begin(), he_crs.end());
      if (hes_crs[hash_id] == he_vec) {
        hes_wts_crs[hash_id]
            = hes_wts_crs[hash_id] + hgraph->hyperedge_weights_[i];
      } else {
        hes_wts_crs.push_back(hgraph->hyperedge_weights_[i]);
        hes_crs.push_back(he_vec);
      }
    }
  }
  return std::make_shared<TPilpGraph>(TPilpGraph(hgraph->vertex_dimensions_,
                                                 hgraph->hyperedge_dimensions_,
                                                 true,
                                                 fixed_vtxs_crs,
                                                 hes_crs,
                                                 vtx_wts_crs,
                                                 hes_wts_crs));
}

void TPilpRefine::SolveIlpInstanceOR(std::shared_ptr<TPilpGraph> hgraph,
                                     TP_partition& refined_partition,
                                     const matrix<float>& max_block_balance)
{
  // reset variable
  refined_partition.clear();
  refined_partition.resize(hgraph->GetNumVertices());
  std::fill(refined_partition.begin(), refined_partition.end(), -1);
  // Google OR-Tools Implementation
  std::unique_ptr<MPSolver> solver(MPSolver::CreateSolver("SCIP"));

  // Define constraints
  // For each vertex, define a variable x
  // For each hyperedge, define a variable y
  std::vector<std::vector<const MPVariable*>> x(
      num_parts_, std::vector<const MPVariable*>(hgraph->GetNumVertices()));
  std::vector<std::vector<const MPVariable*>> y(
      num_parts_, std::vector<const MPVariable*>(hgraph->GetNumHyperedges()));
  // initialize variables
  for (auto& x_v_vector : x)
    for (auto& x_v : x_v_vector)
      x_v = solver->MakeIntVar(
          0.0, 1.0, "");  // represent whether the vertex is within block
  for (auto& y_e_vector : y)
    for (auto& y_e : y_e_vector)
      y_e = solver->MakeIntVar(
          0.0, 1.0, "");  // represent whether the hyperedge is within block
  // define the inifity constant
  const double infinity = solver->infinity();
  for (int i = 0; i < hgraph->GetVertexDimensions(); ++i) {
    // allowed balance for each dimension
    for (int j = 0; j < num_parts_; ++j) {
      MPConstraint* constraint
          = solver->MakeRowConstraint(0.0, max_block_balance[j][i], "");
      for (int k = 0; k < hgraph->GetNumVertices(); k++) {
        auto vwt = hgraph->GetVertexWeight(k);
        constraint->SetCoefficient(x[j][k], vwt[i]);
      }  // finish travering vertices
    }    // finish traversing blocks
  }

  for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
    if (hgraph->CheckFixedStatus(i) == true) {
      MPConstraint* constraint = solver->MakeRowConstraint(1, 1, "");
      constraint->SetCoefficient(x[hgraph->GetFixedPart(i)][i], 1);
    }
  }

  // each vertex can only belong to one part
  for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
    MPConstraint* constraint = solver->MakeRowConstraint(1, 1, "");
    for (int j = 0; j < num_parts_; j++) {
      constraint->SetCoefficient(x[j][i], 1);
    }
  }
  // Hyperedge constraint
  for (int e = 0; e < hgraph->GetNumHyperedges(); ++e) {
    std::pair<int, int> indices = hgraph->GetEdgeIndices(e);
    const int first_valid_entry = indices.first;
    const int first_invalid_entry = indices.second;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int vertex_id = hgraph->eind_[j];
      for (int k = 0; k < num_parts_; ++k) {
        MPConstraint* constraint = solver->MakeRowConstraint(0, infinity, "");
        constraint->SetCoefficient(x[k][vertex_id], 1);
        constraint->SetCoefficient(y[k][e], -1);
      }
    }
  }
  // Maximize cutsize objective
  MPObjective* const obj_expr = solver->MutableObjective();
  for (int i = 0; i < hgraph->GetNumHyperedges(); ++i) {
    auto hwt = hgraph->GetHyperedgeWeight(i);
    const float cost_value = std::inner_product(
        hwt.begin(), hwt.end(), e_wt_factors_.begin(), 0.0);
    for (int j = 0; j < num_parts_; ++j) {
      obj_expr->SetCoefficient(y[j][i], cost_value);
    }
  }
  obj_expr->SetMaximization();

  // Solve the ILP Problem
  const MPSolver::ResultStatus result_status = solver->Solve();
  // Check that the problem has an optimal solution.
  if (result_status == MPSolver::OPTIMAL) {
    for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
      for (int j = 0; j < num_parts_; ++j) {
        if (x[j][i]->solution_value() == 1.0) {
          refined_partition[i] = j;
        }
      }
    }
  }
}

// Updated by Zhiang: 20230206
// We replace CPLEX with the CP-SAT in Google OR-Tools
// We can add hint in CP-SAT to replace warm-start in CPLEX
void TPilpRefine::SolveIlpInstance(std::shared_ptr<TPilpGraph> hgraph,
                                   TP_partition& refined_partition,
                                   const matrix<float>& max_block_balance)
{
  SolveIlpInstanceOR(hgraph, refined_partition, max_block_balance);
  return;
  // Build CP Model
  CpModelBuilder cp_model;
  // Variables
  // x[i][j] is an array of Boolean variables
  // x[i][j] is true if vertex i to partition j
  std::vector<std::vector<BoolVar>> var_x(
      num_parts_, std::vector<BoolVar>(hgraph->GetNumVertices()));
  std::vector<std::vector<BoolVar>> var_y(
      num_parts_, std::vector<BoolVar>(hgraph->GetNumHyperedges()));
  for (auto i = 0; i < num_parts_; i++) {
    // initialize var_x
    for (auto j = 0; j < hgraph->GetNumVertices(); j++)
      var_x[i][j] = cp_model.NewBoolVar();
    // initialize var_y
    for (auto j = 0; j < hgraph->GetNumHyperedges(); j++)
      var_y[i][j] = cp_model.NewBoolVar();
  }
  // define constraints
  // balance constraint
  // check each dimension
  for (int i = 0; i < hgraph->GetVertexDimensions(); ++i) {
    // allowed balance for each dimension
    for (int j = 0; j < num_parts_; ++j) {
      LinearExpr balance_expr;
      for (int k = 0; k < hgraph->GetNumVertices(); ++k) {
        balance_expr += hgraph->GetVertexWeight(k)[i] * var_x[j][k];
      }  // finish traversing vertices
      cp_model.AddLessOrEqual(balance_expr, max_block_balance[j][i]);
    }  // finish traversing blocks
  }    // finish dimension check
  // each vertex can only belong to one part between part_x and part_y
  for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
    if (hgraph->CheckFixedStatus(i) == true) {
      for (auto j = 0; j < num_parts_; j++) {
        cp_model.FixVariable(var_x[j][i], j == hgraph->GetFixedPart(i));
      }  // fixed vertices
    } else {
      std::vector<BoolVar> possible_partitions;
      for (auto j = 0; j < num_parts_; j++) {
        possible_partitions.push_back(var_x[j][i]);
      }
      cp_model.AddExactlyOne(possible_partitions);
    }
  }
  // Hyperedge constraint
  for (int e = 0; e < hgraph->GetNumHyperedges(); ++e) {
    std::pair<int, int> indices = hgraph->GetEdgeIndices(e);
    const int first_valid_entry = indices.first;
    const int first_invalid_entry = indices.second;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int vertex_id = hgraph->eind_[j];
      for (int k = 0; k < num_parts_; ++k) {
        cp_model.AddLessOrEqual(var_y[k][e], var_x[k][vertex_id]);
      }
    }
  }
  // Objective (Maximize objective function -> Minimize cutsize)
  LinearExpr obj_expr;
  for (int i = 0; i < hgraph->GetNumHyperedges(); ++i) {
    auto ewt = hgraph->GetHyperedgeWeight(i);
    const float cost_value = std::inner_product(
        ewt.begin(), ewt.end(), e_wt_factors_.begin(), 0.0);
    for (int j = 0; j < num_parts_; ++j) {
      obj_expr += var_y[j][i] * cost_value;
    }
  }
  cp_model.Maximize(obj_expr);
  // solve
  const CpSolverResponse response = Solve(cp_model.Build());
  // Print solution.
  if (response.status() == CpSolverStatus::INFEASIBLE) {
    logger_->report(
        "No feasible solution found with ILP --> Running K-way FM instead");
  } else {
    for (auto i = 0; i < hgraph->GetNumVertices(); i++) {
      for (auto j = 0; j < num_parts_; j++) {
        if (SolutionBooleanValue(response, var_x[j][i])) {
          refined_partition[i] = j;
        }
      }
    }
  }
  // close the model
}

/*
void TPilpRefine::SolveIlpInstance(std::shared_ptr<TPilpGraph> hgraph,
                                   TP_partition& refined_partition,
                                   const matrix<float>& max_block_balance)
{
  IloEnv myenv;
  IloModel mymodel(myenv);
  IloArray<IloNumVarArray> x(myenv, num_parts_);
  IloArray<IloNumVarArray> y(myenv, num_parts_);
  struct comp
  {
    // comparator function
    bool operator()(const std::pair<int, float>& l,
                    const std::pair<int, float>& r) const
    {
      if (l.second != r.second)
        return l.second > r.second;
      return l.first < r.first;
    }
  };
  for (int i = 0; i < num_parts_; ++i) {
    x[i] = IloNumVarArray(myenv, hgraph->GetNumVertices(), 0, 1, ILOINT);
    y[i] = IloNumVarArray(myenv, hgraph->GetNumHyperedges(), 0, 1, ILOINT);
  }
  if (hgraph->CheckFixedFlag() == true) {
    for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
      if (hgraph->CheckFixedStatus(i) == true) {
        mymodel.add(x[hgraph->GetFixedPart(i)][i] == 1);
      }
    }
  }
  // define constraints
  // balance constraint
  // check each dimension
  for (int i = 0; i < hgraph->GetVertexDimensions(); ++i) {
    // allowed balance for each dimension
    for (int j = 0; j < num_parts_; ++j) {
      IloExpr balance_expr(myenv);
      for (int k = 0; k < hgraph->GetNumVertices(); ++k) {
        auto vwt = hgraph->GetVertexWeight(k);
        balance_expr += vwt[i] * x[j][k];
      }  // finish traversing vertices
      mymodel.add(IloRange(myenv, 0.0, balance_expr, max_block_balance[j][i]));
      balance_expr.end();
    }  // finish traversing blocks
  }    // finish dimension check
  // each vertex can only belong to one part between part_x and part_y
  for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
    if (hgraph->CheckFixedStatus(i) == true) {
      continue;
    } else {
      IloExpr vertex_expr(myenv);
      vertex_expr += x[0][i];
      vertex_expr += x[1][i];
      mymodel.add(vertex_expr == 1);
      vertex_expr.end();
    }
  }
  // Hyperedge constraint
  for (int e = 0; e < hgraph->GetNumHyperedges(); ++e) {
    std::pair<int, int> indices = hgraph->GetEdgeIndices(e);
    const int first_valid_entry = indices.first;
    const int first_invalid_entry = indices.second;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int vertex_id = hgraph->eind_[j];
      for (int k = 0; k < num_parts_; ++k) {
        mymodel.add(y[k][e] <= x[k][vertex_id]);
      }
    }
  }
  // Maximize cutsize objective
  IloExpr obj_expr(myenv);  // empty expression
  for (int i = 0; i < hgraph->GetNumHyperedges(); ++i) {
    auto ewt = hgraph->GetHyperedgeWeight(i);
    const float cost_value = std::inner_product(
        ewt.begin(), ewt.end(), e_wt_factors_.begin(), 0.0);
    for (int j = 0; j < num_parts_; ++j) {
      obj_expr += cost_value * y[j][i];
    }
  }
  mymodel.add(IloMaximize(myenv, obj_expr));  // adding minimization objective
  obj_expr.end();                             // clear memory
  // Model Solution
  IloCplex mycplex(myenv);
  mycplex.extract(mymodel);
  mycplex.setOut(myenv.getNullStream());
  // mycplex.setParam(IloCplex::Param::MIP::Limits::Solutions, 5);
  // mycplex.setParam(IloCplex::Param::TimeLimit, 5);
  mycplex.solve();
  IloBool feasible = mycplex.solve();
  if (feasible == IloTrue) {
    for (int i = 0; i < hgraph->GetNumVertices(); ++i) {
      for (int j = 0; j < num_parts_; ++j) {
        if (mycplex.getValue(x[j][i]) == 1.00) {
          refined_partition[i] = j;
        }
      }
    }
    // some solution may invalid due to the limitation of ILP solver
    for (auto& value : refined_partition)
      value = (value == -1) ? 0 : value;
  } else {
    // for Ilp infeasibility debug
    //mycplex.exportModel("model.mps");
    // DebugIlpInstance("model.mps");
  }
  // closing the model
  mycplex.clear();
  myenv.end();
}
*/

inline void TPilpRefine::Remap(std::vector<int>& partition,
                               std::vector<int>& refined_partition)
{
  for (int i = 0; i < cluster_map_.size(); ++i) {
    partition[i] = refined_partition[cluster_map_[i]];
  }
}

void TPilpRefine::Refine(const HGraph hgraph,
                         const matrix<float>& max_vertex_balance,
                         TP_partition& solution)
{
  matrix<int> net_degs = GetNetDegrees(hgraph, solution);
  std::pair<int, int> partition_pair = std::make_pair(0, 1);
  std::vector<int> boundary_vertices
      = FindBoundaryVertices(hgraph, net_degs, partition_pair);
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  OrderVertexSet(hgraph,
                 boundary_vertices,
                 solution,
                 net_degs,
                 block_balance,
                 max_vertex_balance);
  int boundary_n = boundary_vertices.size();
  int wavefront = std::min(boundary_n, 50);
  std::vector<int> wavefront_vertices(boundary_vertices.begin(),
                                      boundary_vertices.begin() + wavefront);
  ContractNonBoundary(hgraph, wavefront_vertices, solution);
  auto clustered_hg_ilp = ContractHypergraph(hgraph, solution, wavefront);
  std::vector<int> partition(clustered_hg_ilp->GetNumVertices(), -1);
  SolveIlpInstance(clustered_hg_ilp, partition, max_vertex_balance);
  if (*std::min_element(partition.begin(), partition.end()) > -1) {
    Remap(solution, partition);
  } else {
    logger_->report("[ilp refinement failed]");
  }
}

// KPM based FM implementation starts here
/*
// Calculates weight of connections between two partitions
float TPkpm::CalculateSpan(const HGraph hgraph,
                           int& from_pid,
                           int& to_pid,
                           std::vector<int>& solution)
{
  float span = 0.0;
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    const int first_valid_entry = hgraph->eptr_[i];
    const int first_invalid_entry = hgraph->eptr_[i + 1];
    bool flag_partition_from = false;
    bool flag_partition_to = false;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      const int v_id = hgraph->eind_[j];
      if (solution[v_id] == from_pid) {
        flag_partition_from = true;
      } else if (solution[v_id] == to_pid) {
        flag_partition_to = true;
      }
    }
    if (flag_partition_from == true && flag_partition_to == true) {
      span += std::inner_product(hgraph->hyperedge_weights_[i].begin(),
                                 hgraph->hyperedge_weights_[i].end(),
                                 e_wt_factors_.begin(),
                                 0.0);
    }
  }
  return span;
}

void TPkpm::FindTightlyConnectedPairs(const HGraph hgraph,
                                      std::vector<int>& solution)
{
  int id = -1;
  std::vector<TP_partition_pair_ptr> pairs;
  for (int i = 0; i < num_parts_; ++i) {
    for (int j = i + 1; j < num_parts_; ++j) {
      float connection = CalculateSpan(hgraph, i, j, solution);
      auto pair = std::make_shared<TPpartitionPair>(++id, i, j, connection);
      pairs.push_back(pair);
    }
  }
  std::vector<int8_t> pair_visited(num_parts_, 0);
  auto pair_comp = [&](TP_partition_pair_ptr x, TP_partition_pair_ptr y) {
    return x->GetConnectivity > y->GetConnectivity();
  } std::sort(pairs.begin(), pairs.end(), pair_comp);
  std::vector<std::pair<int, int>> kpm_pairs;
  for (int i = 0; i < pairs.size(); ++i) {
    auto kpm_pair = pairs[i];
    int pair_x = kpm_pair->GetPairX();
    int pair_y = kpm_pair->GetPairY();
    if (pair_visited[pair_x] == 1 || pair_visited[pair_y] == 1) {
      continue;
    }
    kpm_pairs.push_back(std::make_pair(pair_x, pair_y));
  }
}
*/
}  // namespace par

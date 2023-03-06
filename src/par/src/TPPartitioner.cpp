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
#include "TPPartitioner.h"

#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

// for ILP solver in google OR-Tools
#include <absl/flags/flag.h>
#include <absl/strings/match.h>
#include <absl/strings/string_view.h>
#include <ortools/base/commandlineflags.h>
//#include <ortools/base/init_google.h>
#include <ortools/base/logging.h>
#include <ortools/linear_solver/linear_solver.h>
#include <ortools/linear_solver/linear_solver.pb.h>

#include "ortools/base/logging.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"

// removed CPLEX
// for ILP solver in CPLEX
// #include "ilcplex/cplex.h"
// #include "ilcplex/ilocplex.h"

using utl::PAR;

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

TP_partition_token TPpartitioner::GoldenEvaluator(const HGraph hgraph,
                                                  std::vector<int>& solution,
                                                  bool print_flag)
{
  matrix<float> block_balance = GetBlockBalance(hgraph, solution);
  float cost = 0.0;
  // check the cutsize
  for (int e = 0; e < hgraph->num_hyperedges_; ++e) {
    for (int idx = hgraph->eptr_[e] + 1; idx < hgraph->eptr_[e + 1]; ++idx) {
      if (solution[hgraph->eind_[idx]] != solution[hgraph->eind_[idx - 1]]) {
        cost += hgraph->edge_score(e, e_wt_factors_);
        break;  // this net has been cut
      }
    }  // finish hyperedge e
  }
  // check timing paths
  for (int path_id = 0; path_id < hgraph->num_timing_paths_; path_id++)
    cost += CalculatePathCost(path_id, hgraph, solution);
  // check if the solution is valid
  for (auto value : solution)
    if (value < 0 || value >= num_parts_)
      logger_->report("Error! The solution is invalid!");
  if (print_flag == true) {
    // print cost
    logger_->report("[Cutcost of partition : {}]", cost);
    // print block balance
    std::vector<float> tot_vertex_weights = hgraph->GetTotalVertexWeights();
    for (auto block_id = 0; block_id < block_balance.size(); block_id++) {
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
  return std::pair<float, std::vector<std::vector<float>>>(cost, block_balance);
}

void TPpartitioner::TimingCutsEvaluator(const HGraph hgraph,
                                        std::vector<int>& solution)
{
  int total_cuts = 0;
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
    total_cuts += cut_on_path;
    if (cut_on_path > worst_cut) {
      worst_cut = cut_on_path;
    }
    if (cut_on_path > 0) {
      ++total_critical_paths_cut;
    }
  }
  logger_->report("[INFO] Total critical paths cut {}",
                  total_critical_paths_cut);
  logger_->report("[INFO] Worst cut on a critical path {}", worst_cut);
}

// Get block balance
matrix<float> TPpartitioner::GetBlockBalance(const HGraph hgraph,
                                             std::vector<int>& solution)
{
  std::vector<std::vector<float>> block_balance(
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

float TPpartitioner::CalculatePathCost(int path_id,
                                       const HGraph hgraph,
                                       const std::vector<int>& solution,
                                       int v,
                                       int to_pid)
{
  float cost = 0.0;  // cost for current path
  if (hgraph->num_timing_paths_ == 0)
    return cost;  // no timing paths

  std::vector<int> path;             // represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  for (auto idx = hgraph->vptr_p_[path_id]; idx < hgraph->vptr_p_[path_id];
       ++idx) {
    const int u = hgraph->vind_p_[idx];  // current vertex
    const int block_id = (u == v) ? to_pid : solution[u];
    if (path.size() == 0 || path.back() != block_id) {
      path.push_back(block_id);
      if (block_counter.find(block_id) != block_counter.end())
        block_counter[block_id] += 1;
      else
        block_counter[block_id] = 1;
    }
  }

  if (path.size() <= 1)
    return cost;

  // num_cut = path.size() - 1
  // cost = path_wt_factor_ * static_cast<float>(path.size() - 1);
  cost = path_wt_factor_
         * std::pow(hgraph->timing_attr_[path_id],
                    2.0 * static_cast<float>(path.size() - 1));
  // get the snaking factor of the path (maximum repetition of block_id - 1)
  int snaking_factor = 0;
  for (auto [block_id, count] : block_counter)
    if (count > snaking_factor)
      snaking_factor = count;
  cost += snaking_wt_factor_ * static_cast<float>(snaking_factor - 1);
  return cost;
}

void TPpartitioner::Partition(const HGraph hgraph,
                              const matrix<float>& max_block_balance,
                              std::vector<int>& solution)
{
  const PartitionType partitioner = GetPartitionerChoice();
  if (partitioner == PartitionType::INIT_RANDOM) {
    RandomPart(hgraph, max_block_balance, solution);
  } else if (partitioner == PartitionType::INIT_DIRECT_ILP) {
    OptimalPartCplexWarmStart(hgraph, max_block_balance, solution);
    // OptimalPartCplex(hgraph, max_block_balance, solution);
  } else if (partitioner == PartitionType::INIT_VILE) {
    InitPartVileTwoWay(hgraph, max_block_balance, solution);
  }
}

// Random Partition:
// Randomly shuffle the vertices and assign it to a block
void TPpartitioner::RandomPart(const HGraph hgraph,
                               const matrix<float>& max_block_balance,
                               std::vector<int>& solution)
{
  // reset variable
  solution.clear();
  solution.resize(hgraph->num_vertices_);
  std::fill(solution.begin(), solution.end(), -1);
  // the summation of vertex weights for vertices in current block
  matrix<float> block_balance(
      num_parts_, std::vector<float>(hgraph->vertex_dimensions_, 0.0f));
  // determine all the free vertices
  std::vector<int> unvisited;

  // if timing paths are present then shuffle vertices according to the path
  // groups they belong to a path group is defined as the superset of vertices
  // of all path graphs

  if (hgraph->num_timing_paths_ > 0) {
    // Fill in order of vertices in the paths
    std::set<int> path_vertices;
    std::vector<int> visited(hgraph->num_vertices_, 0);
    for (int i = 0; i < hgraph->num_timing_paths_; ++i) {
      const int first_valid_entry = hgraph->vptr_p_[i];
      const int first_invalid_entry = hgraph->vptr_p_[i + 1];
      for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
        int v = hgraph->vind_p_[j];
        visited[v] = 1;
        path_vertices.insert(v);
      }
    }
    for (int i = 0; i < hgraph->num_vertices_; ++i) {
      if (visited[i] == 0) {
        unvisited.push_back(i);
      }
    }
    // random shuffle
    shuffle(
        unvisited.begin(), unvisited.end(), std::default_random_engine(seed_));
    unvisited.insert(
        unvisited.begin(), path_vertices.begin(), path_vertices.end());
  } else {
    if (hgraph->fixed_vertex_flag_ == true) {
      for (auto v = 0; v < hgraph->num_vertices_; ++v) {
        if (hgraph->fixed_attr_[v] > -1) {
          solution[v] = hgraph->fixed_attr_[v];
          block_balance[solution[v]]
              = block_balance[solution[v]] + hgraph->vertex_weights_[v];
        } else {
          unvisited.push_back(v);
        }
      }  // done traversal
    } else {
      unvisited.resize(hgraph->num_vertices_);
      std::iota(unvisited.begin(), unvisited.end(), 0);  // Fill with 0, 1, ...
    }
    // random shuffle
    shuffle(
        unvisited.begin(), unvisited.end(), std::default_random_engine(seed_));
  }

  // assign vertex to blocks
  int block_id = 0;
  for (auto v : unvisited) {
    solution[v] = block_id;
    block_balance[solution[v]]
        = block_balance[solution[v]] + hgraph->vertex_weights_[v];
    if (block_balance[block_id]
        >= DivideFactor(max_block_balance[block_id], 10.0)) {
      ++block_id;
      block_id = block_id % num_parts_;  // adjust the block_id
    }
  }
}

void TPpartitioner::InitPartVileTwoWay(const HGraph hgraph,
                                       const matrix<float>& max_block_balance,
                                       std::vector<int>& solution)
{
  // Fill partition 0 with all vertices
  std::fill(solution.begin(), solution.end(), 0);
  // Balance partition with FM
  tritonpart_two_way_refiner_->BalancePartition(
      hgraph, max_block_balance, solution);
  tritonpart_two_way_refiner_->Refine(hgraph, max_block_balance, solution);
}

void TPpartitioner::InitPartVileKWay(const HGraph hgraph,
                                     const matrix<float>& max_block_balance,
                                     std::vector<int>& solution)
{
  // Fill partition 0 with all vertices
  std::fill(solution.begin(), solution.end(), 0);
}

// CP with warm start and hyperedge reduction
// Optimal ILP-based partitioning using CP-SAT
void TPpartitioner::OptimalPartCplexWarmStart(
    const HGraph hgraph,
    const matrix<float>& max_block_balance,
    std::vector<int>& solution)
{
  // TODO: This code is disabled as CP-SAT is non-deterministic when
  // threaded (see https://github.com/google/or-tools/discussions/3275).
  // This may have some QOR degradation to be quantified.
  logger_->report(
      "Optimal ILP-based Partitioning (OR-Tools) with warm-start ...");
  logger_->report(
      "Before Hyperedge Reduction :  num_vertices = {}, num_hyperedges = {}",
      hgraph->num_vertices_,
      hgraph->num_hyperedges_);
  // find the top 10 % most critical hyperedges
  std::vector<int> edge_mask;  // store the hyperedges being used.
  std::set<int> vertex_mask;   // store the vertices being used
  // define comp structure to compare hyperedge ( function: >)
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

  // Here we blow up the hyperedge cost to avoid the unstability due to
  // conversion accuray We reserve the 6 digits after numeric point
  const float blow_factor = 1000000.0;
  float tot_cost = 0.0;  // total hyperedge cut
  // use set data structure to sort hyperedges
  std::set<std::pair<int, float>, comp> unvisited_hyperedges;
  for (auto e = 0; e < hgraph->num_hyperedges_; ++e) {
    const float score = hgraph->edge_score(e, e_wt_factors_);
    unvisited_hyperedges.insert(std::pair<int, float>(e, score));
    tot_cost += score;
  }
  // set the reserved total score for reduced hypergraph (100% for now)
  const float reserved_tot_cost = tot_cost * 1.0;
  float cur_total_score = 0.0;
  for (auto& value : unvisited_hyperedges) {
    if (cur_total_score <= reserved_tot_cost) {
      edge_mask.push_back(value.first);
      cur_total_score += value.second;
      const int& e = value.first;  // hyperedge id
      // check if the hyperedge is a single-vertex hyperedge
      // the existance of single-vertex hyperedge cannot lead to multiple
      // optimal solutions
      if (hgraph->eptr_[e + 1] - hgraph->eptr_[e] <= 1)
        continue;
      for (auto eptr_id = hgraph->eptr_[e]; eptr_id < hgraph->eptr_[e + 1];
           eptr_id++) {
        vertex_mask.insert(hgraph->eind_[eptr_id]);
      }
    } else {
      break;  // the set has been sorted
    }
  }
  // To make sure that there is always only one optimal solution,
  // while preserving the best structure
  // the min_diff_score will be added to the vertices within one block
  const float min_diff_score
      = 1.0f / (vertex_mask.size() * vertex_mask.size() * num_parts_);
  logger_->report("min_diff_score : {}", min_diff_score);
  // generate reduced hypergraph and store the mapping relationship
  std::vector<int> old_vertex_vec(hgraph->num_vertices_,
                                  -1);  // map old vertex id to new vertex id
  std::vector<int> new_vertex_vec(vertex_mask.size(),
                                  -1);  // map new vertex id to old vertex id
  std::vector<std::set<int>> new_hyperedges;  // new hyperedges
  const int num_vertices = vertex_mask.size();
  const int num_hyperedges = edge_mask.size();
  // create new vertices
  int vertex_id = 0;
  for (auto& old_vertex : vertex_mask) {
    old_vertex_vec[old_vertex] = vertex_id;
    new_vertex_vec[vertex_id++] = old_vertex;
  }
  // create new hyperedges
  for (auto& e : edge_mask) {
    std::set<int> hyperedge;
    for (auto eptr_id = hgraph->eptr_[e]; eptr_id < hgraph->eptr_[e + 1];
         eptr_id++) {
      hyperedge.insert(old_vertex_vec[hgraph->eind_[eptr_id]]);
    }
    // Note that here we do not need to detect whether the hyperegde is a
    // single-vertex hyperedge Because we just rename the vertices, thus the
    // dimensionality of each hyperedge will not change
    new_hyperedges.push_back(hyperedge);
  }
  // print basic information
  logger_->report(
      "After Hyperedge Reduction :  num_vertices = {}, num_hyperedges = {}",
      num_vertices,
      num_hyperedges);

  // From here, we are going to create CP-SAT model for the reduced hypergraph
  matrix<int> x(num_parts_, std::vector<int>(num_vertices, 0));
  matrix<int> y(num_parts_, std::vector<int>(num_hyperedges, 0));
  // set hints for vertices and hyperedges
  // set vertex v to its partition solution
  for (auto v = 0; v < num_vertices; v++) {
    const int part_id = solution[new_vertex_vec[v]];
    for (auto i = 0; i < num_parts_; i++) {
      x[i][v] = (i == part_id) ? 1 : 0;
    }
  }
  // set hyperedge e to its partition solution
  // if hyperedge e is fully with partition i, set y[i][e] = 1
  // otherwise y[i][e] = 0
  for (auto e = 0; e < num_hyperedges; e++) {
    std::set<int> unique_partitions;
    for (auto& v : new_hyperedges[e]) {
      unique_partitions.insert(solution[new_vertex_vec[v]]);
    }
    if (unique_partitions.size() == 1) {
      const int part_id = *unique_partitions.begin();
      for (int i = 0; i < num_parts_; i++) {
        y[i][e] = (i == part_id) ? 1 : 0;
      }
    } else {
      for (int i = 0; i < num_parts_; i++) {
        y[i][e] = 0;
      }
    }
  }

  // Build CP Model
  CpModelBuilder cp_model;
  // Variables
  // var_x[i][j] is an array of Boolean variables
  // var_x[i][j] is true if vertex i to partition j
  std::vector<std::vector<BoolVar>> var_x(num_parts_,
                                          std::vector<BoolVar>(num_vertices));
  std::vector<std::vector<BoolVar>> var_y(num_parts_,
                                          std::vector<BoolVar>(num_hyperedges));
  for (auto i = 0; i < num_parts_; i++) {
    // initialize var_x
    for (auto v = 0; v < num_vertices; v++) {
      var_x[i][v] = cp_model.NewBoolVar();
    }
    // initialize var_y
    for (auto e = 0; e < num_hyperedges; e++) {
      var_y[i][e] = cp_model.NewBoolVar();
    }
  }

  // define constraints
  // balance constraint
  // check each dimension
  for (auto k = 0; k < hgraph->vertex_dimensions_; k++) {
    // allowed balance for each dimension
    for (auto i = 0; i < num_parts_; i++) {
      LinearExpr balance_expr;
      for (int v = 0; v < num_vertices; v++) {
        balance_expr
            += hgraph->vertex_weights_[new_vertex_vec[v]][k] * var_x[i][v];
      }
      cp_model.AddLessOrEqual(balance_expr, max_block_balance[i][k]);
    }
  }
  // Fixed vertices constraints
  if (hgraph->fixed_vertex_flag_ == true) {
    for (int v = 0; v < num_vertices; v++) {
      if (hgraph->fixed_attr_[new_vertex_vec[v]] > -1) {
        for (int i = 0; i < num_parts_; i++) {
          cp_model.FixVariable(var_x[i][v],
                               i == hgraph->fixed_attr_[new_vertex_vec[v]]);
        }
      }  // fixed vertices should be placed at the specified block
    }
  }

  // each vertex can only belong to one part
  for (auto v = 0; v < num_vertices; v++) {
    std::vector<BoolVar> possible_partitions;
    for (auto i = 0; i < num_parts_; i++) {
      possible_partitions.push_back(var_x[i][v]);
    }
    cp_model.AddExactlyOne(possible_partitions);
  }

  // Hyperedge constraint
  for (auto e = 0; e < num_hyperedges; e++) {
    for (auto v : new_hyperedges[e]) {
      for (int i = 0; i < num_parts_; i++) {
        cp_model.AddLessOrEqual(var_y[i][e], var_x[i][v]);
      }
    }
  }

  // Objective (Maximize objective function -> Minimize cutsize)
  LinearExpr obj_expr;
  for (auto e = 0; e < num_hyperedges; e++) {
    const float cost_value = hgraph->edge_score(edge_mask[e], e_wt_factors_);
    for (int i = 0; i < num_parts_; ++i) {
      // obj_expr += var_y[i][e] * cost_value ;
      obj_expr += var_y[i][e] * static_cast<int>(cost_value * blow_factor)
                  * num_vertices * num_parts_;
    }
    for (int v = 0; v < num_vertices; v++) {
      for (int i = 0; i < num_parts_; i++) {
        obj_expr -= var_x[i][v] * (i * num_vertices * num_vertices + v);
      }
    }
  }
  cp_model.Maximize(obj_expr);

  // Add hints
  for (auto i = 0; i < num_parts_; i++) {
    // hint for var_x
    for (auto v = 0; v < num_vertices; v++) {
      cp_model.AddHint(var_x[i][v], x[i][v]);
    }
    // hint for var_y
    for (auto e = 0; e < num_hyperedges; e++) {
      cp_model.AddHint(var_y[i][e], y[i][e]);
    }
  }

  // solve
  const CpSolverResponse response = Solve(cp_model.Build());
  // Print solution.
  if (response.status() == CpSolverStatus::OPTIMAL) {
    for (auto v = 0; v < num_vertices; v++) {
      for (auto i = 0; i < num_parts_; i++) {
        if (SolutionBooleanValue(response, var_x[i][v])) {
          solution[new_vertex_vec[v]] = i;
        }
      }
    }
  } else {
    logger_->report(
        "No feasible solution found with ILP --> Running K-way FM instead");
  }
  // close the model
}

// Commented out by Zhiang (20230203, 5:01PM)
// Now Zhiang is replacing it by CP in Google OR-Tools
// CPLEX with warm start
/*
void TPpartitioner::OptimalPartCplexWarmStart(
    const HGraph hgraph,
    const matrix<float>& max_block_balance,
    std::vector<int>& solution)
{
  matrix<int> x(num_parts_, std::vector<int>(hgraph->num_vertices_, 0));
  matrix<int> y(num_parts_, std::vector<int>(hgraph->num_hyperedges_, 0));
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    int p = solution[i];
    x[p][i] = 1;
  }
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    int firstValidEntry = hgraph->eptr_[i];
    int firstInvalidEntry = hgraph->eptr_[i + 1];
    std::set<int> unique_partitions;
    for (int j = firstValidEntry; j < firstInvalidEntry; ++j) {
      int v = hgraph->eind_[j];
      int p = solution[v];
      unique_partitions.insert(p);
    }
    for (const int& j : unique_partitions) {
      y[j][i] = 1;
    }
  }
  IloEnv myenv;
  IloModel mymodel(myenv);
  IloNumVarArray startVarX(myenv);
  IloNumArray startValX(myenv);
  IloArray<IloNumVarArray> _x(myenv, num_parts_);
  IloArray<IloNumVarArray> _y(myenv, num_parts_);
  for (int i = 0; i < num_parts_; ++i) {
    _x[i] = IloNumVarArray(myenv, hgraph->num_vertices_, 0, 1, ILOINT);
    _y[i] = IloNumVarArray(myenv, hgraph->num_hyperedges_, 0, 1, ILOINT);
  }
  for (int i = 0; i < num_parts_; ++i) {
    for (int j = 0; j < hgraph->num_vertices_; ++j) {
      startVarX.add(_x[i][j]);
      startValX.add(x[i][j]);
    }
  }
  // define constraints
  // balance constraint
  // check each dimension
  for (int i = 0; i < hgraph->vertex_dimensions_; ++i) {
    // allowed balance for each dimension
    for (int j = 0; j < num_parts_; ++j) {
      IloExpr balance_expr(myenv);
      for (int k = 0; k < hgraph->num_vertices_; ++k) {
        balance_expr += hgraph->vertex_weights_[k][i] * _x[j][k];
      }  // finish traversing vertices
      mymodel.add(IloRange(myenv, 0.0, balance_expr, max_block_balance[j][i]));
      balance_expr.end();
    }  // finish traversing blocks
  }    // finish dimension check
  // Fixed vertices constraint
  if (hgraph->fixed_vertex_flag_ == true) {
    for (int i = 0; i < hgraph->num_vertices_; ++i) {
      if (hgraph->fixed_attr_[i] > -1) {
        mymodel.add(_x[hgraph->fixed_attr_[i]][i] == 1);
      }  // fixed vertices should be placed at the specified block
    }
  }
  // each vertex can only belong to one part
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    IloExpr vertex_expr(myenv);
    for (int j = 0; j < num_parts_; ++j) {
      vertex_expr += _x[j][i];
    }
    mymodel.add(vertex_expr == 1);
    vertex_expr.end();
  }
  // Hyperedge constraint
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    const int start_idx = hgraph->eptr_[i];
    const int end_idx = hgraph->eptr_[i + 1];
    for (int j = start_idx; j < end_idx; j++) {
      const int vertex_id = hgraph->eind_[j];
      for (int k = 0; k < num_parts_; ++k) {
        mymodel.add(_y[k][i] <= _x[k][vertex_id]);
      }
    }
  }
  // Maximize cutsize objective
  IloExpr obj_expr(myenv);  // empty expression
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    const float cost_value = hgraph->edge_score(i, e_wt_factors_);
    for (int j = 0; j < num_parts_; ++j) {
      obj_expr += cost_value * _y[j][i];
    }
  }
  mymodel.add(IloMaximize(myenv, obj_expr));  // adding minimization objective
  obj_expr.end();                             // clear memory
  // Model Solution
  IloCplex mycplex(myenv);
  mycplex.extract(mymodel);
  // mycplex.setParam(IloCplex::Param::Preprocessing::Presolve, 0);
  // mycplex.setParam(IloCplex::Param::Read::Scale, 1);
  // mycplex.setParam(IloCplex::Param::Simplex::Tolerances::Markowitz, 0.99);
  mycplex.setOut(myenv.getNullStream());
  mycplex.addMIPStart(
      startVarX, startValX, IloCplex::MIPStartAuto, "secondMIPStart");
  mycplex.setParam(IloCplex::Param::MIP::Limits::Solutions, 2);
  startVarX.end();
  startValX.end();
  mycplex.solve();
  IloBool feasible = mycplex.solve();
  if (feasible == IloTrue) {
    for (int i = 0; i < hgraph->num_vertices_; i++) {
      for (int j = 0; j < num_parts_; j++) {
        if (mycplex.getValue(_x[j][i]) == 1.00) {
          solution[i] = j;
        }
      }
    }
    // some solution may invalid due to the limitation of ILP solver
    for (auto& value : solution)
      value = (value == -1) ? 0 : value;
  } else {
    std::cout
        << "No feasible solution found with ILP --> Running K-way FM instead"
        << std::endl;
    // DirectKWayFM(hgraph, max_block_balance, solution);
  }
  // closing the model
  mycplex.clear();
  myenv.end();
}

// CPLEX based ILP Solver
void TPpartitioner::OptimalPartCplex(const HGraph hgraph,
                                     const matrix<float>& max_block_balance,
                                     std::vector<int>& solution)
{
  std::cout << "Enter OptimalPartCplex" << std::endl;
  // reset variable
  solution.clear();
  solution.resize(hgraph->num_vertices_);
  std::fill(solution.begin(), solution.end(), -1);
  // CPLEX-Based Implementation
  // define environment
  IloEnv myenv;
  IloModel mymodel(myenv);
  // Define constraints
  // For each vertex, define a variable x
  // For each hyperedge, define a variable y
  IloArray<IloNumVarArray> x(myenv, num_parts_);
  IloArray<IloNumVarArray> y(myenv, num_parts_);

  // only consider the top 100 hyperedges based on weight
  // order hyperedges based on decreasing order
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

  // use set data structure to sort unvisited vertices
  std::set<std::pair<int, float>, comp> unvisited_hyperedges;
  for (auto e = 0; e < hgraph->num_hyperedges_; ++e) {
    const float score = hgraph->edge_score(e, e_wt_factors_);
    unvisited_hyperedges.insert(std::pair<int, float>(e, score));
  }

  int max_num_hyperedges = 0;
  std::vector<int> edge_mask;
  float base_score = (*unvisited_hyperedges.begin()).second / 10.0;
  for (auto& value : unvisited_hyperedges) {
    edge_mask.push_back(value.first);
    max_num_hyperedges++;
  }
  for (int i = 0; i < num_parts_; ++i) {
    x[i] = IloNumVarArray(myenv, hgraph->num_vertices_, 0, 1, ILOINT);
    y[i] = IloNumVarArray(myenv, edge_mask.size(), 0, 1, ILOINT);
  }

  // define constraints
  // balance constraint
  // check each dimension
  for (int i = 0; i < hgraph->vertex_dimensions_; ++i) {
    // allowed balance for each dimension
    for (int j = 0; j < num_parts_; ++j) {
      IloExpr balance_expr(myenv);
      for (int k = 0; k < hgraph->num_vertices_; ++k) {
        balance_expr += hgraph->vertex_weights_[k][i] * x[j][k];
      }  // finish traversing vertices
      mymodel.add(IloRange(myenv, 0.0, balance_expr, max_block_balance[j][i]));
      balance_expr.end();
    }  // finish traversing blocks
  }    // finish dimension check
  // Fixed vertices constraint
  if (hgraph->fixed_vertex_flag_ == true) {
    for (int i = 0; i < hgraph->num_vertices_; ++i) {
      if (hgraph->fixed_attr_[i] > -1) {
        mymodel.add(x[hgraph->fixed_attr_[i]][i] == 1);
      }  // fixed vertices should be placed at the specified block
    }
  }
  // each vertex can only belong to one part
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    IloExpr vertex_expr(myenv);
    for (int j = 0; j < num_parts_; ++j) {
      vertex_expr += x[j][i];
    }
    mymodel.add(vertex_expr == 1);
    vertex_expr.end();
  }
  // Hyperedge constraint
  for (int i = 0; i < edge_mask.size(); ++i) {
    const int e = edge_mask[i];
    const int start_idx = hgraph->eptr_[e];
    const int end_idx = hgraph->eptr_[e + 1];
    for (int j = start_idx; j < end_idx; j++) {
      const int vertex_id = hgraph->eind_[j];
      for (int k = 0; k < num_parts_; k++) {
        mymodel.add(y[k][i] <= x[k][vertex_id]);
      }
    }
  }
  // Maximize cutsize objective
  IloExpr obj_expr(myenv);  // empty expression
  for (int i = 0; i < edge_mask.size(); ++i) {
    const float cost_value = hgraph->edge_score(edge_mask[i], e_wt_factors_);
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
  mycplex.solve();
  IloBool feasible = mycplex.solve();
  if (feasible == IloTrue) {
    for (int i = 0; i < hgraph->num_vertices_; ++i) {
      for (int j = 0; j < num_parts_; ++j) {
        if (mycplex.getValue(x[j][i]) == 1.00) {
          solution[i] = j;
        }
      }
    }
    // some solution may invalid due to the limitation of ILP solver
    for (auto& value : solution)
      value = (value == -1) ? 0 : value;
  } else {
    std::cout
        << "No feasible solution found with ILP --> Running K-way FM instead"
        << std::endl;
    // DirectKWayFM(hgraph, max_block_balance, solution);
  }
  // closing the model
  mycplex.clear();
  myenv.end();
}
*/
}  // namespace par

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
#include "Refiner.h"

#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

using utl::PAR;

VertexGain::VertexGain(const int vertex,
                       const int src_block_id,
                       const int destination_block_id,
                       const float gain,
                       const std::map<int, float>& path_cost)
    : vertex_(vertex),
      source_part_(src_block_id),
      destination_part_(destination_block_id),
      gain_(gain),
      path_cost_(path_cost)
{
}

HyperedgeGain::HyperedgeGain(const int hyperedge_id,
                             const int destination_part,
                             const float gain,
                             const std::map<int, float>& path_cost)
    : hyperedge_id_(hyperedge_id),
      destination_part_(destination_part),
      gain_(gain),
      path_cost_(path_cost)
{
}

Refiner::Refiner(
    const int num_parts,
    const int refiner_iters,
    const float path_wt_factor,     // weight for cutting a critical timing path
    const float snaking_wt_factor,  // weight for snaking timing paths
    const int max_move,      // the maximum number of vertices or hyperedges can
                             // be moved in each pass
    EvaluatorPtr evaluator,  // evaluator
    utl::Logger* logger)
    : num_parts_(num_parts),
      refiner_iters_(refiner_iters),
      path_wt_factor_(path_wt_factor),
      snaking_wt_factor_(snaking_wt_factor),
      max_move_(max_move),
      refiner_iters_default_(refiner_iters),
      max_move_default_(max_move)
{
  evaluator_ = std::move(evaluator);
  logger_ = logger;
}

void Refiner::SetMaxMove(const int max_move)
{
  debugPrint(logger_, PAR, "refinement", 1, "Set the max_move to {}", max_move);
  max_move_ = max_move;
}

void Refiner::SetRefineIters(const int refiner_iters)
{
  debugPrint(logger_,
             PAR,
             "refinement",
             1,
             "Set the refiner_iter to {}",
             refiner_iters);
  refiner_iters_ = refiner_iters;
}

void Refiner::RestoreDefaultParameters()
{
  max_move_ = max_move_default_;
  refiner_iters_ = refiner_iters_default_;
  debugPrint(
      logger_, PAR, "refinement", 1, "Reset the max_move to {}", max_move_);
  debugPrint(logger_,
             PAR,
             "refinement",
             1,
             "Reset the refiner_iters to {}",
             refiner_iters_);
}

// The main function of refinement class
void Refiner::Refine(const HGraphPtr& hgraph,
                     const Matrix<float>& upper_block_balance,
                     const Matrix<float>& lower_block_balance,
                     Partitions& solution)
{
  if (max_move_ <= 0) {
    debugPrint(logger_,
               PAR,
               "refinement",
               1,
               "max_move = {}. Exiting Refinement.",
               max_move_);
    return;
  }
  // calculate the basic statistics of current solution
  Matrix<float> cur_block_balance
      = evaluator_->GetBlockBalance(hgraph, solution);
  Matrix<int> net_degs = evaluator_->GetNetDegrees(hgraph, solution);
  std::vector<float> cur_paths_cost;
  if (hgraph->HasTiming()) {
    cur_paths_cost = evaluator_->GetPathsCost(hgraph, solution);
  }
  for (int i = 0; i < refiner_iters_; ++i) {
    // the main function for improving the solution
    // mark the vertices can be moved as unvisited
    std::vector<bool> visited_vertices_flag(hgraph->GetNumVertices(), false);
    // mark all fixed vertices as visited vertices
    if (hgraph->HasFixedVertices()) {
      for (auto v = 0; v < hgraph->GetNumVertices(); v++) {
        if (hgraph->GetFixedAttr(v) > -1) {
          visited_vertices_flag[v] = true;
        }
      }
    }
    const float gain = Pass(hgraph,
                            upper_block_balance,
                            lower_block_balance,
                            cur_block_balance,
                            net_degs,
                            cur_paths_cost,
                            solution,
                            visited_vertices_flag);
    if (gain <= 0.0) {
      return;  // stop if there is no improvement
    }
  }
}

// ---------------------------------------------------------------
// Protected functions
// ---------------------------------------------------------------

// By default, v = -1 and to_pid = -1
// if to_pid == -1, we are calculate the current cost
// of the path;
// else if to_pid != -1, we are culculate the cost of the path
// after moving v to block to_pid
float Refiner::CalculatePathCost(int path_id,
                                 const HGraphPtr& hgraph,
                                 const Partitions& solution,
                                 int v,      // v = -1 by default
                                 int to_pid  // to_pid = -1 by default
) const
{
  float cost = 0.0;  // cost for current path
  if (hgraph->GetNumTimingPaths() == 0
      || path_id >= hgraph->GetNumTimingPaths()) {
    return cost;  // no timing paths
  }
  // we must use vector here.  Becuase we need to calculate the snaking factor
  std::vector<int> path;             // represent the path in terms of block_id
  std::map<int, int> block_counter;  // block_id counter
  for (const int u : hgraph->PathVertices(path_id)) {
    int block_id = solution[u];
    if ((to_pid != -1) && (u == v)) {
      block_id = to_pid;
    }
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
    return cost;
  }
  // timing-related cost (basic path_cost * number of cut on the path)
  cost = path_wt_factor_ * (path.size() - 1) * hgraph->PathTimingCost(path_id);
  // get the snaking factor of the path (maximum repetition of block_id - 1)
  int snaking_factor = 0;
  for (auto [block_id, count] : block_counter) {
    if (count > snaking_factor) {
      snaking_factor = count;
    }
  }
  cost += snaking_wt_factor_ * static_cast<float>(snaking_factor - 1);
  return cost;
}

// Find all the boundary vertices.
// The boundary vertices do not include fixed vertices
std::vector<int> Refiner::FindBoundaryVertices(
    const HGraphPtr& hgraph,
    const Matrix<int>& net_degs,
    const std::vector<bool>& visited_vertices_flag) const
{
  // Step 1 : found all the boundary hyperedges
  std::vector<bool> boundary_net_flag(hgraph->GetNumHyperedges(), false);
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    int num_span_part = 0;
    for (int i = 0; i < num_parts_; i++) {
      if (net_degs[e][i] > 0) {
        num_span_part++;
        if (num_span_part >= 2) {
          boundary_net_flag[e] = true;
          break;
        }
      }
    }
  }
  // Step 2: check all the non-fixed vertices
  std::vector<int> boundary_vertices;
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    if (visited_vertices_flag[v] == true) {
      continue;  // This vertex has been visited
    }
    for (const int edge_id : hgraph->Edges(v)) {
      if (boundary_net_flag[edge_id]) {
        boundary_vertices.push_back(v);
        break;
      }
    }
  }
  return boundary_vertices;
}

std::vector<int> Refiner::FindBoundaryVertices(
    const HGraphPtr& hgraph,
    const Matrix<int>& net_degs,
    const std::vector<bool>& visited_vertices_flag,
    const std::vector<int>& solution,
    const std::pair<int, int>& partition_pair) const
{
  // Step 1 : found all the boundary hyperedges
  std::vector<bool> boundary_net_flag(hgraph->GetNumHyperedges(), false);
  for (int e = 0; e < hgraph->GetNumHyperedges(); e++) {
    if (net_degs[e][partition_pair.first] > 0
        && net_degs[e][partition_pair.second] > 0) {
      boundary_net_flag[e] = true;
    }
  }
  // Step 2: check all the non-fixed vertices
  std::vector<int> boundary_vertices;
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    if (visited_vertices_flag[v] == true) {
      continue;
    }
    for (const int edge_id : hgraph->Edges(v)) {
      if (boundary_net_flag[edge_id]) {
        boundary_vertices.push_back(v);
        break;
      }
    }
  }
  return boundary_vertices;
}

// Find the neighboring vertices
std::vector<int> Refiner::FindNeighbors(
    const HGraphPtr& hgraph,
    const int vertex_id,
    const std::vector<bool>& visited_vertices_flag) const
{
  std::set<int> neighbors;
  for (const int e : hgraph->Edges(vertex_id)) {
    for (const int v : hgraph->Vertices(e)) {
      if (visited_vertices_flag[v] == false) {
        // This vertex has not been visited yet
        neighbors.insert(v);
      }
    }
  }
  return std::vector<int>(neighbors.begin(), neighbors.end());
}

// Find the neighboring vertices in specified blocks
std::vector<int> Refiner::FindNeighbors(
    const HGraphPtr& hgraph,
    const int vertex_id,
    const std::vector<bool>& visited_vertices_flag,
    const std::vector<int>& solution,
    const std::pair<int, int>& partition_pair) const
{
  std::set<int> neighbors;
  for (const int e : hgraph->Edges(vertex_id)) {
    for (const int v : hgraph->Vertices(e)) {
      if (visited_vertices_flag[v] == false
          && (solution[v] == partition_pair.first
              || solution[v] == partition_pair.second)) {
        // This vertex has not been visited yet
        neighbors.insert(v);
      }
    }
  }
  return std::vector<int>(neighbors.begin(), neighbors.end());
}

// Functions related to move a vertex
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
// cur_path_cost : current path cost
// net_degs : current net degrees
GainCell Refiner::CalculateVertexGain(int v,
                                      int from_pid,
                                      int to_pid,
                                      const HGraphPtr& hgraph,
                                      const std::vector<int>& solution,
                                      const std::vector<float>& cur_paths_cost,
                                      const Matrix<int>& net_degs) const
{
  // We assume from_pid == solution[v] when we call CalculateGain
  // we need solution argument to update the score related to path
  float cut_score = 0.0;
  float path_score = 0.0;
  std::map<int, float>
      delta_path_cost;       // map path_id to the change of path cost
  if (from_pid == to_pid) {  // no gain for this case
    return std::make_shared<VertexGain>(
        v, from_pid, to_pid, 0.0f, delta_path_cost);
  }
  // define lambda function
  // for checking connectivity (number of blocks connected by a hyperedge)
  // function : check the connectivity for the hyperedge
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto& num_v : net_degs[e]) {
      if (num_v > 0) {
        connectivity++;
      }
    }
    return connectivity;
  };
  // traverse all the hyperedges connected to v
  for (const int e : hgraph->Edges(v)) {
    const int connectivity = GetConnectivity(e);
    const float e_score = evaluator_->CalculateHyperedgeCost(e, hgraph);
    if (connectivity == 0) {
      // ignore the hyperedge consisting of multiple vertices
      // ignore single-vertex hyperedge
      continue;
    }
    if (connectivity == 1 && net_degs[e][from_pid] > 1) {
      // move from_pid to to_pid will have negative score
      // all the vertices are with block from_id
      cut_score -= e_score;
    } else if (connectivity == 2 && net_degs[e][from_pid] == 1
               && net_degs[e][to_pid] > 0) {
      // all the vertices excluding v are all within block to_pid
      // move from_pid to to_pid will increase the score
      cut_score += e_score;
    }
  }
  // check the timing path
  if (hgraph->GetNumTimingPaths() > 0) {
    for (const int path_id : hgraph->TimingPathsThrough(v)) {
      // Get updated path costs if vertex is moved to a different partition
      const float cost
          = CalculatePathCost(path_id, hgraph, solution, v, to_pid);
      delta_path_cost[path_id] = cost - cur_paths_cost[path_id];
      // gain accomodates for the change in the cost of the timing path
      path_score += cur_paths_cost[path_id] - cost;  // score in minus cost
    }
  }
  const float score = cut_score + path_score;
  return std::make_shared<VertexGain>(
      v, from_pid, to_pid, score, delta_path_cost);
}

// move one vertex based on the calculated gain_cell
void Refiner::AcceptVertexGain(const GainCell& gain_cell,
                               const HGraphPtr& hgraph,
                               float& total_delta_gain,
                               std::vector<bool>& visited_vertices_flag,
                               std::vector<int>& solution,
                               std::vector<float>& cur_paths_cost,
                               Matrix<float>& curr_block_balance,
                               Matrix<int>& net_degs) const
{
  const int vertex_id = gain_cell->GetVertex();
  visited_vertices_flag[vertex_id] = true;
  total_delta_gain += gain_cell->GetGain();  // increase the total gain
  // Update the path cost first
  for (const auto& [path_id, delta_path_cost] : gain_cell->GetPathCost()) {
    cur_paths_cost[path_id] += delta_path_cost;
  }
  // get partition id
  const int pre_part_id = gain_cell->GetSourcePart();
  const int new_part_id = gain_cell->GetDestinationPart();
  // update the solution vector
  solution[vertex_id] = new_part_id;
  // Update the partition balance
  curr_block_balance[pre_part_id]
      = curr_block_balance[pre_part_id] - hgraph->GetVertexWeights(vertex_id);
  curr_block_balance[new_part_id]
      = curr_block_balance[new_part_id] + hgraph->GetVertexWeights(vertex_id);
  // update net_degs
  for (const int he : hgraph->Edges(vertex_id)) {
    --net_degs[he][pre_part_id];
    ++net_degs[he][new_part_id];
  }
}

// restore one vertex based on the calculated gain_cell
void Refiner::RollBackVertexGain(const GainCell& gain_cell,
                                 const HGraphPtr& hgraph,
                                 std::vector<bool>& visited_vertices_flag,
                                 std::vector<int>& solution,
                                 std::vector<float>& cur_paths_cost,
                                 Matrix<float>& curr_block_balance,
                                 Matrix<int>& net_degs) const
{
  const int vertex_id = gain_cell->GetVertex();
  visited_vertices_flag[vertex_id] = false;
  // Update the path cost first
  for (const auto& [path_id, delta_path_cost] : gain_cell->GetPathCost()) {
    cur_paths_cost[path_id] -= delta_path_cost;
  }
  // get partition id
  const int pre_part_id = gain_cell->GetSourcePart();
  const int new_part_id = gain_cell->GetDestinationPart();
  // update the solution vector
  solution[vertex_id] = pre_part_id;
  // Update the partition balance
  curr_block_balance[pre_part_id]
      = curr_block_balance[pre_part_id] + hgraph->GetVertexWeights(vertex_id);
  curr_block_balance[new_part_id]
      = curr_block_balance[new_part_id] - hgraph->GetVertexWeights(vertex_id);
  // update net_degs
  for (const int he : hgraph->Edges(vertex_id)) {
    ++net_degs[he][pre_part_id];
    --net_degs[he][new_part_id];
  }
}

// check if we can move the vertex to some block
// Here we assume the vertex v is not in the block to_pid
bool Refiner::CheckVertexMoveLegality(
    int v,         // vertex id
    int to_pid,    // to block id
    int from_pid,  // from block_id
    const HGraphPtr& hgraph,
    const Matrix<float>& curr_block_balance,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance) const
{
  const std::vector<float> total_wt_to_block
      = curr_block_balance[to_pid] + hgraph->GetVertexWeights(v);
  const std::vector<float> total_wt_from_block
      = curr_block_balance[from_pid] - hgraph->GetVertexWeights(v);
  return total_wt_to_block <= upper_block_balance[to_pid]
         && lower_block_balance[from_pid] <= total_wt_from_block;
}

// calculate the possible gain of moving a entire hyperedge
// We can view the process of moving the vertices in hyperege
// one by one, then restore the moving sequence to make sure that
// the current status is not changed. Solution should not be const
HyperedgeGainPtr Refiner::CalculateHyperedgeGain(
    int hyperedge_id,
    int to_pid,
    const HGraphPtr& hgraph,
    std::vector<int>& solution,
    const std::vector<float>& cur_paths_cost,
    const Matrix<int>& net_degs) const
{
  // We assume from_pid == solution[v] when we call CalculateGain
  // we need solution argument to update the score related to path
  float cut_score = 0.0;
  float path_score = 0.0;
  float score = 0.0;
  std::map<int, float>
      delta_path_cost;  // map path_id to the change of path cost
  // find the all the vertices of hyperedge,
  // which are not in the to_pid block
  std::vector<std::pair<int, int>> vertices;  // vertex_id, from_pid
  // We need to modify these net degrees
  std::map<int, std::vector<int>> net_deg_map;
  for (const int vertex_id : hgraph->Vertices(hyperedge_id)) {
    if (solution[vertex_id] != to_pid) {
      vertices.emplace_back(vertex_id, solution[vertex_id]);
      for (const int e : hgraph->Edges(vertex_id)) {
        if (net_deg_map.find(e) == net_deg_map.end()) {
          net_deg_map[e] = net_degs[e];
        }
      }
    }
  }
  if (vertices.empty() == true) {
    return std::make_shared<HyperedgeGain>(
        hyperedge_id, to_pid, score, delta_path_cost);
  }
  // define lambda function
  // for checking connectivity (number of blocks connected by a hyperedge)
  // function : check the connectivity for the hyperedge
  auto GetConnectivity = [&](int e) {
    int connectivity = 0;
    for (auto& num_v : net_deg_map[e]) {
      if (num_v > 0) {
        connectivity++;
      }
    }
    return connectivity;
  };
  // check all the vertices
  // Step 1: check the cut cost
  for (const auto& vertex_pair : vertices) {
    const int v = vertex_pair.first;
    const int from_pid = vertex_pair.second;
    // traverse all the hyperedges connected to v
    for (const int e : hgraph->Edges(v)) {
      const int connectivity = GetConnectivity(e);
      const float e_score = evaluator_->CalculateHyperedgeCost(e, hgraph);
      if (connectivity == 0) {
        // ignore the hyperedge consisting of multiple vertices
        // ignore single-vertex hyperedge
        continue;
      }
      if (connectivity == 1 && net_deg_map[e][from_pid] > 1) {
        // move from_pid to to_pid will have negative score
        // all the vertices are with block from_id
        cut_score -= e_score;
      } else if (connectivity == 2 && net_deg_map[e][from_pid] == 1
                 && net_deg_map[e][to_pid] > 0) {
        // all the vertices excluding v are all within block to_pid
        // move from_pid to to_pid will increase the score
        cut_score += e_score;
      }
      net_deg_map[e][from_pid]--;
      net_deg_map[e][to_pid]++;
    }
  }
  // Step 2: check timing cost
  if (hgraph->GetNumTimingPaths() > 0) {
    // update the solution to to_pid
    for (const auto& vertex_pair : vertices) {
      solution[vertex_pair.first] = to_pid;
    }
    for (const auto& vertex_pair : vertices) {
      const int v = vertex_pair.first;
      for (const int path_id : hgraph->TimingPathsThrough(v)) {
        if (delta_path_cost.find(path_id) == delta_path_cost.end()) {
          // Get updated path costs if vertex is moved to a different partition
          const float cost
              = CalculatePathCost(path_id, hgraph, solution, v, to_pid);
          delta_path_cost[path_id] = cost - cur_paths_cost[path_id];
          // gain accomodates for the change in the cost of the timing path
          path_score += cur_paths_cost[path_id] - cost;  // score in minus cost
        }
      }
    }
    // restore the solution to from_id
    for (const auto& vertex_pair : vertices) {
      solution[vertex_pair.first] = vertex_pair.second;
    }
  }

  score = cut_score + path_score;
  return std::make_shared<HyperedgeGain>(
      hyperedge_id, to_pid, score, delta_path_cost);
}

// accpet the hyperedge gain
void Refiner::AcceptHyperedgeGain(const HyperedgeGainPtr& hyperedge_gain,
                                  const HGraphPtr& hgraph,
                                  float& total_delta_gain,
                                  std::vector<int>& solution,
                                  std::vector<float>& cur_paths_cost,
                                  Matrix<float>& cur_block_balance,
                                  Matrix<int>& net_degs) const
{
  const int hyperedge_id = hyperedge_gain->GetHyperedge();
  total_delta_gain += hyperedge_gain->GetGain();
  // Update the path cost first
  for (const auto& [path_id, delta_path_cost] : hyperedge_gain->GetPathCost()) {
    cur_paths_cost[path_id] += delta_path_cost;
  }
  // get block id
  const int new_part_id = hyperedge_gain->GetDestinationPart();
  // update the solution vector block_balance and net_degs
  for (const int vertex_id : hgraph->Vertices(hyperedge_id)) {
    const int pre_part_id = solution[vertex_id];
    if (pre_part_id == new_part_id) {
      continue;  // the vertex is in current block
    }
    // update solution
    solution[vertex_id] = new_part_id;
    // Update the partition balance
    cur_block_balance[pre_part_id]
        = cur_block_balance[pre_part_id] - hgraph->GetVertexWeights(vertex_id);
    cur_block_balance[new_part_id]
        = cur_block_balance[new_part_id] + hgraph->GetVertexWeights(vertex_id);
    // update net_degs
    // not just this hyperedge, we need to update all the related hyperedges
    for (const int he : hgraph->Edges(vertex_id)) {
      --net_degs[he][pre_part_id];
      ++net_degs[he][new_part_id];
    }
  }
}

bool Refiner::CheckHyperedgeMoveLegality(
    int e,       // hyperedge id
    int to_pid,  // to block id
    const HGraphPtr& hgraph,
    const std::vector<int>& solution,
    const Matrix<float>& curr_block_balance,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance) const
{
  Matrix<float> update_block_balance = curr_block_balance;
  for (const int v : hgraph->Vertices(e)) {
    // check if satisfies the fixed vertices constraint
    if (hgraph->HasFixedVertices() && hgraph->GetFixedAttr(v) != to_pid) {
      return false;  // violate the fixed vertices constraint
    }
    const int pid = solution[v];
    if (solution[v] != to_pid) {
      update_block_balance[to_pid]
          = update_block_balance[to_pid] + hgraph->GetVertexWeights(v);
      update_block_balance[pid]
          = update_block_balance[pid] - hgraph->GetVertexWeights(v);
    }
  }
  // Violate the upper bound
  if (upper_block_balance[to_pid] < update_block_balance[to_pid]) {
    return false;
  }
  // Violate the lower bound
  for (int pid = 0; pid < num_parts_; pid++) {
    if (pid != to_pid) {
      if (update_block_balance[pid] < lower_block_balance[pid]) {
        return false;
      }
    }
  }
  // valid move
  return true;
}

}  // namespace par

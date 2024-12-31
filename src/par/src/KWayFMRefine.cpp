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
#include "KWayFMRefine.h"

#include <functional>
#include <limits>
#include <memory>
#include <set>
#include <thread>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "PriorityQueue.h"
#include "Refiner.h"
#include "Utilities.h"
#include "utl/Logger.h"

// Implement the direct k-way FM refinement
namespace par {

KWayFMRefine::KWayFMRefine(const int num_parts,
                           const int refiner_iters,
                           const float path_wt_factor,
                           const float snaking_wt_factor,
                           const int max_move,
                           const int total_corking_passes,
                           EvaluatorPtr evaluator,
                           utl::Logger* logger)
    : Refiner(num_parts,
              refiner_iters,
              path_wt_factor,
              snaking_wt_factor,
              max_move,
              std::move(evaluator),
              logger),
      total_corking_passes_(total_corking_passes)
{
}

// In each pass, we only move the boundary vertices
float KWayFMRefine::Pass(
    const HGraphPtr& hgraph,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance,
    Matrix<float>& block_balance,        // the current block balance
    Matrix<int>& net_degs,               // the current net degree
    std::vector<float>& cur_paths_cost,  // the current path cost
    Partitions& solution,
    std::vector<bool>& visited_vertices_flag)
{
  // initialize the gain buckets
  GainBuckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // the maxinum size of each bucket is hgraph->GetNumVertices()
    auto bucket = std::make_shared<PriorityQueue>(
        hgraph->GetNumVertices(), total_corking_passes_, hgraph);
    buckets.push_back(bucket);
  }
  // identify all the boundary vertices
  // fixed vertices will not be identified as boundary vertices
  std::vector<int> boundary_vertices
      = FindBoundaryVertices(hgraph, net_degs, visited_vertices_flag);
  if (boundary_vertices.empty() == true) {
    return 0.0f;  // no vertices are available
  }
  // Initialize current gain in a multi-thread manner
  // set based on max heap (k set)
  // each block has its own max heap
  InitializeGainBucketsKWay(
      buckets, hgraph, boundary_vertices, net_degs, cur_paths_cost, solution);
  // Here we do not store the vertex directly,
  // because we need to restore the status to the status with   best_gain
  // Based on our experiments, the moves is usually very limited.
  // Restoring from backwards will be more efficient
  std::vector<GainCell> moves_trace;  // store the moved vertex_gain in sequence
  float total_delta_gain = 0.0;
  // Trick here:  We can adjust the best_gain to value to decide whether we
  // should accept a worse solution If the current solution violates the balance
  // constraint, we have to accept the worse solution to get a balanced solution
  // Otherwise we should only accept better solutions
  float best_gain = 0.0;
  for (int block_id = 0; block_id < num_parts_; block_id++) {
    if (upper_block_balance[block_id] < block_balance[block_id]
        || block_balance[block_id] < lower_block_balance[block_id]) {
      best_gain = -std::numeric_limits<float>::max();
      break;
    }
  }

  int best_vertex_id = -1;  // dummy best vertex id
  // main loop of FM pass
  for (int i = 0; i < max_move_; i++) {
    auto candidate = PickMoveKWay(buckets,
                                  hgraph,
                                  block_balance,
                                  upper_block_balance,
                                  lower_block_balance);
    // check the status of candidate
    const int vertex = candidate->GetVertex();  // candidate vertex
    if (vertex < 0) {
      break;  // no valid vertex found
    }
    AcceptKWayMove(candidate,
                   buckets,
                   moves_trace,
                   total_delta_gain,
                   visited_vertices_flag,
                   hgraph,
                   block_balance,
                   net_degs,
                   cur_paths_cost,
                   solution);
    std::vector<int> neighbors
        = FindNeighbors(hgraph, vertex, visited_vertices_flag);
    // update the neighbors of v for all gain buckets in parallel
    std::vector<std::thread> threads;
    threads.reserve(num_parts_);
    for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
      threads.emplace_back(&KWayFMRefine::UpdateSingleGainBucket,
                           this,
                           to_pid,
                           std::ref(buckets),
                           hgraph,
                           std::ref(neighbors),
                           std::ref(net_degs),
                           std::ref(cur_paths_cost),
                           std::ref(solution));
    }
    for (auto& t : threads) {
      t.join();  // wait for all threads to finish
    }
    threads.clear();
    if (total_delta_gain >= best_gain) {
      best_gain = total_delta_gain;
      best_vertex_id = vertex;
    }
  }

  // find the best solution and restore the status which achieves the best
  // solution traverse the moves_trace in the reversing order
  for (auto move_iter = moves_trace.rbegin(); move_iter != moves_trace.rend();
       move_iter++) {
    // stop when we encounter the best_vertex_id
    auto& vertex_move = *move_iter;
    if (vertex_move->GetVertex() == best_vertex_id) {
      break;  // stop here
    }
    RollBackVertexGain(vertex_move,
                       hgraph,
                       visited_vertices_flag,
                       solution,
                       cur_paths_cost,
                       block_balance,
                       net_degs);
  }

  // clear the move traces
  moves_trace.clear();
  // clear the buckets
  for (auto block_id = 0; block_id < num_parts_; block_id++) {
    buckets[block_id]->Clear();
  }

  return best_gain;
}

// gain bucket related functions

// Initialize the gain buckets in parallel
void KWayFMRefine::InitializeGainBucketsKWay(
    GainBuckets& buckets,
    const HGraphPtr& hgraph,
    const std::vector<int>& boundary_vertices,
    const Matrix<int>& net_degs,
    const std::vector<float>& cur_paths_cost,
    const Partitions& solution) const
{
  std::vector<std::thread> threads;  // for parallel updating
  threads.reserve(num_parts_);
  // parallel initialize the num_parts gain_buckets
  for (int to_pid = 0; to_pid < num_parts_; to_pid++) {
    threads.emplace_back(
        &KWayFMRefine::InitializeSingleGainBucket,
        this,
        std::ref(buckets),
        to_pid,
        hgraph,
        std::ref(boundary_vertices),  // we only consider boundary vertices
        std::ref(net_degs),
        std::ref(cur_paths_cost),
        std::ref(solution));
  }
  for (auto& t : threads) {
    t.join();  // wait for all threads to finish
  }
  threads.clear();
}

// Initialize the single bucket
void KWayFMRefine::InitializeSingleGainBucket(
    GainBuckets& buckets,
    int to_pid,  // move the vertex into this block (block_id = to_pid)
    const HGraphPtr& hgraph,
    const std::vector<int>& boundary_vertices,
    const Matrix<int>& net_degs,
    const std::vector<float>& cur_paths_cost,
    const Partitions& solution) const
{
  // set current bucket to active
  buckets[to_pid]->SetActive();
  // traverse the boundary vertices
  for (const int& v : boundary_vertices) {
    const int from_part = solution[v];
    if (from_part == to_pid) {
      continue;  // the boundary vertex is the current bucket
    }
    auto gain_cell = CalculateVertexGain(
        v, from_part, to_pid, hgraph, solution, cur_paths_cost, net_degs);
    buckets[to_pid]->InsertIntoPQ(gain_cell);
  }
  // if the current bucket is empty, set the bucket to deactive
  if (buckets[to_pid]->GetTotalElements() == 0) {
    buckets[to_pid]->SetDeactive();
  }
}

// Determine which vertex gain to be picked
std::shared_ptr<VertexGain> KWayFMRefine::PickMoveKWay(
    GainBuckets& buckets,
    const HGraphPtr& hgraph,
    const Matrix<float>& curr_block_balance,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance) const
{
  // dummy candidate
  int to_pid = -1;
  auto candidate = std::make_shared<VertexGain>();

  // best gain bucket for "corking effect".
  // i.e., if there is no normal candidate available,
  // we will traverse the best_to_pid bucket
  int best_to_pid = -1;  // block id with best_gain
  float best_gain = -std::numeric_limits<float>::max();
  auto dummy_cell = std::make_shared<VertexGain>();

  // checking the first elements in each bucket
  for (int i = 0; i < num_parts_; ++i) {
    if (buckets[i]->GetStatus() == false) {
      continue;  // This bucket is empty
    }
    auto ele = buckets[i]->GetMax();
    const int vertex = ele->GetVertex();
    const float gain = ele->GetGain();
    const int from_pid = ele->GetSourcePart();
    if ((gain > candidate->GetGain())
        && CheckVertexMoveLegality(vertex,
                                   i,
                                   from_pid,
                                   hgraph,
                                   curr_block_balance,
                                   upper_block_balance,
                                   lower_block_balance)
               == true) {
      to_pid = i;
      candidate = ele;
    }
    // record part for solving corking effect
    if (gain > best_gain) {
      best_gain = gain;
      best_to_pid = i;
    }
  }
  // Case 1:  if there is a candidate available or no vertex to move
  if (to_pid > -1 || best_to_pid == -1) {
    return candidate;
  }
  // Case 2:  "corking effect", i.e., no candidate
  return buckets.at(best_to_pid)
      ->GetBestCandidate(
          curr_block_balance, upper_block_balance, lower_block_balance, hgraph);
}

// move one vertex based on the calculated gain_cell
void KWayFMRefine::AcceptKWayMove(const std::shared_ptr<VertexGain>& gain_cell,
                                  GainBuckets& gain_buckets,
                                  std::vector<GainCell>& moves_trace,
                                  float& total_delta_gain,
                                  std::vector<bool>& visited_vertices_flag,
                                  const HGraphPtr& hgraph,
                                  Matrix<float>& curr_block_balance,
                                  Matrix<int>& net_degs,
                                  std::vector<float>& cur_paths_cost,
                                  std::vector<int>& solution) const
{
  const int vertex_id = gain_cell->GetVertex();
  moves_trace.push_back(gain_cell);
  AcceptVertexGain(gain_cell,
                   hgraph,
                   total_delta_gain,
                   visited_vertices_flag,
                   solution,
                   cur_paths_cost,
                   curr_block_balance,
                   net_degs);
  // Remove vertex from all buckets where vertex is present
  std::vector<std::thread> deletion_threads;
  deletion_threads.reserve(num_parts_);
  for (int i = 0; i < num_parts_; ++i) {
    deletion_threads.emplace_back(&par::KWayFMRefine::HeapEleDeletion,
                                  this,
                                  vertex_id,
                                  i,
                                  std::ref(gain_buckets));
  }
  for (auto& th : deletion_threads) {
    th.join();
  }
}

// Remove vertex from a heap
// Remove the vertex id related vertex gain
void KWayFMRefine::HeapEleDeletion(int vertex_id,
                                   int part,
                                   GainBuckets& buckets) const
{
  buckets[part]->Remove(vertex_id);
}

// After moving one vertex, the gain of its neighbors will also need
// to be updated. This function is used to update the gain of neighbor vertices
// notices that the neighbors has been calculated based on solution, visited
// status, boundary vertices status
void KWayFMRefine::UpdateSingleGainBucket(
    int part,
    GainBuckets& buckets,
    const HGraphPtr& hgraph,
    const std::vector<int>& neighbors,
    const Matrix<int>& net_degs,
    const std::vector<float>& cur_paths_cost,
    const Partitions& solution) const
{
  std::set<int> neighboring_hyperedges;
  for (const int& v : neighbors) {
    const int from_part = solution[v];
    if (from_part == part) {
      continue;
    }
    // recalculate the current gain of the vertex v
    auto gain_cell = CalculateVertexGain(
        v, from_part, part, hgraph, solution, cur_paths_cost, net_degs);
    // check if the vertex exists in current bucket
    if (buckets[part]->CheckIfVertexExists(v) == true) {
      // update the bucket with new gain
      buckets[part]->ChangePriority(v, gain_cell);
    } else {
      buckets[part]->InsertIntoPQ(gain_cell);
    }
  }
}

}  // namespace par

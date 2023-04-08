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
#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

// Implement the direct k-way FM refinement
namespace par {

// In each pass, we only move the boundary vertices
float TPkWayFM::Pass(const HGraph hgraph,
                     const matrix<float>& max_block_balance,
                     TP_partition& solution)
{
  // precompute the statistics of the solution
  matrix<float> block_balance = evaluator_->GetBlockBalance(hgraph, solution);
  matrix<int> net_degs = evaluator_->GetNetDegrees(hgraph, solution);
  std::vector<float> paths_cost = evaluator_->GetPathCost(hgraph, solution);
  // initialize the gain buckets
  TP_gain_buckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // the maxinum size of each bucket is hgraph->num_vertices_
    TP_gain_bucket bucket
        = std::make_shared<TPpriorityQueue>(hgraph->num_vertices_, 
                                            total_corking_passes_,
                                            hgraph);
    buckets.push_back(bucket);
  }
  // identify all the boundary vertices
  // fixed vertices will not be identified as boundary vertices
  std::vector<int> boundary_vertices = FindBoundaryVertices(hgraph, net_degs);
  // Initialize current gain in a multi-thread manner
  // set based on max heap (k set)
  // each block has its own max heap
  InitializeGainBucketsKWay(
      hgraph, solution, net_degs, boundary_vertices, paths_cost, buckets);
  // set the initial status
  const std::vector<int> pre_fm_solution = solution; // store the original solution
  std::vector<int> moves_trace;  // store the moved vertices in sequence
  float total_delta_gain = 0.0;
  float best_gain = -std::numeric_limits<float>::max();
  int best_vertex_id = -1; // dummy best vertex id
  // main loop of FM pass
  for (int i = 0; i < max_moves_; i++) {
    auto candidate
        = PickMoveKWay(hgraph, buckets, block_balance, max_block_balance);
    // check the status of candidate
    const int vertex = candidate->GetVertex();  // candidate vertex
    if (vertex < 0) {
      break; // no valid vertex found
    }
    AcceptKWayMove(candidate,
                   hgraph,
                   moves_trace,
                   total_delta_gain,
                   solution,
                   paths_cost,
                   block_balance,
                   buckets,
                   net_degs);
    const std::vector<int> neighbors = FindNeighbors(hgraph, vertex, solution);
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
    for (auto& t : threads) {
      t.join();  // wait for all threads to finish
    }
    threads.clear();
    if (total_delta_gain >= best_gain) {
      best_gain = total_delta_gain;
      best_vertex_id = vertex;
    }
  }  

  // find the best gain location
  if (best_gain < 0.0) {
    for (auto& v : moves_trace) {
      solution[v] = pre_fm_solution[v];
    }
  } else {
    bool restore_flag = false;
    for (auto& v : moves_trace) {
      if (restore_flag == true) {
        solution[v] = pre_fm_solution[v]; // restore the original solution
      }
      if (v == best_vertex_id) {
        restore_flag = true; // from here we need to restore the original solution
      }
    }    
  }
}

// gain bucket related functions

// Initialize the gain buckets in parallel
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
                                  boundary_vertices, // we only consider boundary vertices
                                  hgraph,
                                  solution,
                                  cur_path_cost,
                                  net_degs));
  for (auto& t : threads)
    t.join();  // wait for all threads to finish
  threads.clear();
}

// Initialize the single bucket
void TPkWayFM::InitializeSingleGainBucket(
    TP_gain_buckets& buckets,
    int to_pid, // move the vertex into this block (block_id = to_pid)
    const std::vector<int>& boundary_vertices,
    const HGraph hgraph,
    const TP_partition& solution,
    const std::vector<float>& cur_path_cost,
    const matrix<int>& net_degs)
{
  // set current bucket to active
  buckets[to_pid]->SetActive();
  // traverse the boundary vertices
  for (const int& v : boundary_vertices) {
    const int from_part = solution[v];
    if (from_part == to_pid) {
      continue; // the boundary vertex is the current bucket
    } 
    auto gain_cell = CalculateGain(
        v, from_part, to_pid, hgraph, solution, cur_path_cost, net_degs);
    buckets[to_pid]->InsertIntoPQ(gain_cell);
  }
  // if the current bucket is empty, set the bucket to deactive
  if (buckets[to_pid]->GetTotalElements() == 0) {
    buckets[to_pid]->SetDeactive();
  }
}


// Determine which vertex gain to be picked
std::shared_ptr<VertexGain> TPkWayFM::PickMoveKWay(
    const HGraph hgraph,
    TP_gain_buckets& buckets,
    const matrix<float>& curr_block_balance,
    const matrix<float>& max_block_balance)
{
  // dummy candidate
  int to_pid = -1;
  std::shared_ptr<VertexGain> candidate = std::make_shared<VertexGain>();
 
  // best gain bucket for "corking effect".
  // i.e., if there is no normal candidate available,
  // we will traverse the best_to_pid bucket
  int best_to_pid = -1;  // block id with best_gain
  float best_gain = -std::numeric_limits<float>::max();
  std::shared_ptr<VertexGain> dummy_cell = std::make_shared<VertexGain>();

  // checking the first elements in each bucket
  for (int i = 0; i < num_parts_; ++i) {
    if (buckets[i]->GetStatus() == false) {
      continue; // This bucket is empty
    }
    auto ele = buckets[i]->GetMax();
    const int vertex = ele->GetVertex();
    const float gain = ele->GetGain();
    if ((gain > candidate->GetGain())
        && (curr_block_balance[i] + hgraph->vertex_weights_[vertex] < max_block_balance[i])) {
      to_pid = i;
      candidate = ele;
    }
    // record part for solving corking effect
    if (gain > best_gain) {
      best_gain = gain;
      best_to_pid = i;
    }
  }
  // Case 1:  if there is a candidate available
  if (to_pid > -1) {
    return candidate;
  }
  // Case 2:  "corking effect", i.e., no candidate
  return buckets.at(best_to_pid)->GetBestCandidate(curr_block_balance,
                                                   max_block_balance);
}

// move one vertex based on the calculated gain_cell
void TPkWayFM::AcceptKWayMove(std::shared_ptr<VertexGain> gain_cell,
                              HGraph hgraph,
                              std::vector<int>& moves_trace,
                              float& total_delta_gain,
                              std::vector<int>& solution,
                              std::vector<float>& paths_cost,
                              matrix<float>& curr_block_balance,
                              TP_gain_buckets& gain_buckets,
                              matrix<int>& net_degs)
{
  int vertex_id = gain_cell->GetVertex();
  total_delta_gain += gain_cell->GetGain(); // increase the total gain
  moves_trace.push_back(vertex_id);
  visit_[vertex_id] = true; // mark this vertex as visited
  // Update the path cost first
  for (auto& [path_id, path_cost] : gain_cell->GetPathCost()) {
    paths_cost[path_id] = path_cost;
  }  
  // get partition id
  const int pre_part_id = gain_cell->GetSourcePart();
  const int new_part_id = gain_cell->GetDestinationPart();
  // update the solution vector
  solution[vertex_id] = new_part_id;
  // Update the partition balance
  curr_block_balance[pre_part_id]
      = curr_block_balance[pre_part_id] - hgraph->vertex_weights_[vertex_id];
  curr_block_balance[new_part_id]
      = curr_block_balance[new_part_id] + hgraph->vertex_weights_[vertex_id];
  // update net_degs
  const int first_valid_entry = hgraph->vptr_[vertex_id];
  const int first_invalid_entry = hgraph->vptr_[vertex_id + 1];
  for (int i = first_valid_entry; i < first_invalid_entry; ++i) {
    const int he = hgraph->vind_[i]; // hyperedge id
    --net_degs[he][pre_part_id];
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
}

// Remove vertex from a heap
// Remove the vertex id related vertex gain
void TPkWayFM::HeapEleDeletion(int vertex_id,
                               int part,
                               TP_gain_buckets& buckets)
{
  buckets[part]->Remove(vertex_id);
}

// After moving one vertex, the gain of its neighbors will also need
// to be updated. This function is used to update the gain of neighbor vertices
// notices that the neighbors has been calculated based on solution, visited status,
// boundary vertices status
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
    const int from_part = solution[v];
    if (from_part == part) {
      continue;
    }
    // check if the vertex exists in current bucket
    if (buckets[part]->CheckIfVertexExists(v) == true) {
      auto new_gain_cell = CalculateGain(
          v, from_part, part, hgraph, solution, cur_path_cost, net_degs);
      // update the bucket with new gain
      buckets[part]->ChangePriority(v, new_gain_cell);
    } else {
      auto gain_cell = CalculateGain(
          v, from_part, part, hgraph, solution, cur_path_cost, net_degs);
      buckets[part]->InsertIntoPQ(gain_cell);
    }
  }
}

}  // namespace par

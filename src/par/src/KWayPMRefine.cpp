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
#include "KWayPMRefine.h"

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "KWayFMRefine.h"
#include "PriorityQueue.h"
#include "Refiner.h"
#include "Utilities.h"

// ------------------------------------------------------------------------------
// K-way pair-wise FM refinement
// ------------------------------------------------------------------------------
// The motivation is that FM can achieve better performance for 2-way
// partitioning than k-way partitioning. So we decompose the k-way partitioning
// into multiple 2-way partitioning through maximum matching. In the first
// iteration, the maximum matching is based on connectivity between each blocks.
// In the remaing iterations, the maximum matching is based on the delta gain
// for each block. Based on the paper of pair-wise PM, we can not keep using
// connectivity based maximum matching.  Otherwise, we may easily got stuck in
// local minimum. We use multiple multiple member functions of KWayFMRefine
// especially the functions related to gain buckets

namespace par {

// In each pass, we only move the boundary vertices
// here we pass block_balance and net_degrees as reference
// because we only move a few vertices during each pass
// i.e., block_balance and net_degs will not change too much
// so we precompute the block_balance and net_degs
// the return value is the gain improvement
float KWayPMRefine::Pass(
    const HGraphPtr& hgraph,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance,
    Matrix<float>& block_balance,    // the current block balance
    Matrix<int>& net_degs,           // the current net degree
    std::vector<float>& paths_cost,  // the current path cost
    Partitions& solution,
    std::vector<bool>& visited_vertices_flag)
{
  // Step 1: determine the matching score
  std::vector<std::pair<int, int>>
      maximum_matches;  // maximum matching between blocks
  std::map<std::pair<int, int>, float> matching_connectivity
      = evaluator_->GetMatchingConnectivity(hgraph, solution);
  CalculateMaximumMatch(maximum_matches, matching_connectivity);
  float delta_gain = 0.0;
  // Step 2: update the solution based on calculated maximum matching
  // initialize the gain buckets
  GainBuckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // the maxinum size of each bucket is hgraph->GetNumVertices()
    auto bucket = std::make_shared<PriorityQueue>(
        hgraph->GetNumVertices(), total_corking_passes_, hgraph);
    buckets.push_back(bucket);
  }
  for (const auto& partition_pair : maximum_matches) {
    // after performing FM, the corresponding buckets will be cleared
    delta_gain += PerformPairFM(hgraph,
                                upper_block_balance,
                                lower_block_balance,
                                block_balance,
                                net_degs,
                                paths_cost,
                                solution,
                                buckets,
                                visited_vertices_flag,
                                partition_pair);
  }
  return delta_gain;
}

/*
// In each pass, we only move the boundary vertices
// here we pass block_balance and net_degrees as reference
// because we only move a few vertices during each pass
// i.e., block_balance and net_degs will not change too much
// so we precompute the block_balance and net_degs
// the return value is the gain improvement
float KWayPMRefine::Pass(const HGraphPtr hgraph,
                           const Matrix<float>& max_block_balance,
                           Matrix<float>& block_balance, // the current block
balance Matrix<int>& net_degs, // the current net degree std::vector<float>&
paths_cost, // the current path cost Partitions& solution, std::vector<bool>&
visited_vertices_flag)
{
  // Step 1: determine the matching score
  std::vector<std::pair<int, int> > maximum_matches; // maximum matching between
blocks if (pre_matching_connectivity_.empty() == true) {
    // get the connectivity between blocks
    // std::map<std::pair<int, int>, float> : <block_id_a, block_id_b> : score
    // The score is the summation of hyperedges spanning block_id_a and
block_id_b pre_matching_connectivity_ =
evaluator_->GetMatchingConnectivity(hgraph, solution);
    CalculateMaximumMatch(maximum_matches, pre_matching_connectivity_);
  } else {
    std::map<std::pair<int, int>, float> matching_connectivity =
        evaluator_->GetMatchingConnectivity(hgraph, solution);
    // calculate the matching_scores
    std::map<std::pair<int, int>, float> matching_scores;
    for (const auto &ele : matching_connectivity) {
      matching_scores[ele.first] = pre_matching_connectivity_[ele.first]
                                   - matching_connectivity[ele.first];
    }
    pre_matching_connectivity_ = matching_connectivity;
    CalculateMaximumMatch(maximum_matches, matching_scores);
  }

  float delta_gain = 0.0;
  // Step 2: update the solution based on calculated maximum matching
  // initialize the gain buckets
  GainBuckets buckets;
  for (int i = 0; i < num_parts_; ++i) {
    // the maxinum size of each bucket is hgraph->GetNumVertices()
    GainBucket bucket
        = std::make_shared<PriorityQueue>(hgraph->GetNumVertices(),
                                          total_corking_passes_,
                                          hgraph);
    buckets.push_back(bucket);
  }
  for (const auto& partition_pair : maximum_matches) {
    // after performing FM, the corresponding buckets will be cleared
    delta_gain += PerformPairFM(hgraph, max_block_balance, block_balance,
net_degs, paths_cost, solution, buckets, visited_vertices_flag, partition_pair);
  }
  return delta_gain;
}
*/

// The function to calculate the matching_scores
void KWayPMRefine::CalculateMaximumMatch(
    std::vector<std::pair<int, int>>& maximum_matches,
    const std::map<std::pair<int, int>, float>& matching_scores) const
{
  maximum_matches.clear();
  std::vector<std::pair<std::pair<int, int>, float>> scores;
  scores.reserve(matching_scores.size());
  for (const auto& ele : matching_scores) {
    scores.emplace_back(ele);
  }
  // sort the scores based on value
  std::sort(scores.begin(),
            scores.end(),
            // lambda function
            [](const std::pair<std::pair<int, int>, float>& a,
               const std::pair<std::pair<int, int>, float>& b) {
              return a.second > b.second;
            }  // end of lambda expression
  );
  // set the match flag for each block
  std::vector<bool> match_flag(num_parts_, false);
  int num_match_block = 0;
  for (auto& ele : scores) {
    const int block_id_a = ele.first.first;
    const int block_id_b = ele.first.second;
    if (match_flag[block_id_a] == false && match_flag[block_id_b] == false) {
      maximum_matches.emplace_back(block_id_a, block_id_b);
      match_flag[block_id_a] = true;
      match_flag[block_id_b] = true;
      num_match_block += 2;
      if (num_match_block >= num_parts_ - 1) {
        return;  // finish the matching scheme
      }
    }
  }
}

// Perform 2-way FM between blocks in partition pair
float KWayPMRefine::PerformPairFM(
    const HGraphPtr& hgraph,
    const Matrix<float>& upper_block_balance,
    const Matrix<float>& lower_block_balance,
    Matrix<float>& block_balance,    // the current block balance
    Matrix<int>& net_degs,           // the current net degree
    std::vector<float>& paths_cost,  // the current path cost
    Partitions& solution,
    GainBuckets& buckets,
    std::vector<bool>& visited_vertices_flag,
    const std::pair<int, int>& partition_pair) const
{
  // clear the buckets
  std::vector<int> blocks{partition_pair.first, partition_pair.second};
  for (auto& block_id : blocks) {
    buckets[block_id]->Clear();
  }
  // identify all the boundary vertices between partition_pair
  // fixed vertices will not be identified as boundary vertices
  std::vector<int> boundary_vertices = FindBoundaryVertices(
      hgraph, net_degs, visited_vertices_flag, solution, partition_pair);
  // Initialize current gain in a multi-thread manner
  // set based on max heap (k set)
  InitializeGainBucketsPM(buckets,
                          hgraph,
                          boundary_vertices,
                          net_degs,
                          paths_cost,
                          solution,
                          partition_pair);
  // Here we do not store the vertex directly,
  // because we need to restore the status to the status with best_gain
  // Based on our experiments, the moves is usually very limited.
  // Restoring from backwards will be more efficient
  std::vector<GainCell> moves_trace;  // store the moved vertex_gain in sequence
  float total_delta_gain = 0.0;
  // Notice that the best_gain should be initialized as 0 instead of -infinity
  // because after each pass, the total gain should be improved, i.e.,
  // best_gain must be >= 0.0.
  float best_gain = 0.0;
  int best_vertex_id = -1;  // dummy best vertex id
  // main loop of FM pass
  for (int i = 0; i < max_move_; i++) {
    // here we use the PickMoveKWay method inheriting from KWayPMRefine
    // directly, because the buckets cooresponding to other blocks are empty
    // Similarly, we can also use AcceptKWayMove method inheriting from
    // KWayPMRefine
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
                   paths_cost,
                   solution);
    // find the neighbors of vertex in partition_pair blocks
    const std::vector<int> neighbors = FindNeighbors(
        hgraph, vertex, visited_vertices_flag, solution, partition_pair);
    // update the neighbors of v for all gain buckets in parallel
    std::vector<std::thread> threads;
    threads.reserve(blocks.size());
    for (auto& to_pid : blocks) {
      threads.emplace_back(&KWayFMRefine::UpdateSingleGainBucket,
                           this,
                           to_pid,
                           std::ref(buckets),
                           hgraph,
                           std::ref(neighbors),
                           std::ref(net_degs),
                           std::ref(paths_cost),
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

  // traverse the moves_trace in the reversing order
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
                       paths_cost,
                       block_balance,
                       net_degs);
  }

  // clear the move traces
  moves_trace.clear();
  // clear the buckets
  for (auto& block_id : blocks) {
    buckets[block_id]->Clear();
  }

  return best_gain;
}

// gain bucket related functions
// Initialize the gain buckets in parallel
void KWayPMRefine::InitializeGainBucketsPM(
    GainBuckets& buckets,
    const HGraphPtr& hgraph,
    const std::vector<int>& boundary_vertices,
    const Matrix<int>& net_degs,
    const std::vector<float>& cur_paths_cost,
    const Partitions& solution,
    const std::pair<int, int>& partition_pair) const
{
  std::vector<int> blocks_id{partition_pair.first, partition_pair.second};
  std::vector<std::thread> threads;  // for parallel updating
  threads.reserve(blocks_id.size());

  // parallel initialize the num_parts gain_buckets
  for (const auto to_pid : blocks_id) {
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

}  // namespace par

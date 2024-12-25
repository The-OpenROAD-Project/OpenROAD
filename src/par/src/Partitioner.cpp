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

#include "Partitioner.h"

#include <algorithm>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <utility>
#include <vector>

#include "Evaluator.h"
#include "Hypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

using utl::PAR;

Partitioner::Partitioner(int num_parts,
                         int seed,
                         const EvaluatorPtr& evaluator,
                         utl::Logger* logger)
    : num_parts_(num_parts), seed_(seed), evaluator_(evaluator), logger_(logger)
{
}

void Partitioner::EnableIlpAcceleration(float acceleration_factor)
{
  ilp_accelerator_factor_ = acceleration_factor;
  ilp_accelerator_factor_ = std::max(ilp_accelerator_factor_, 0.0f);
  ilp_accelerator_factor_ = std::min(ilp_accelerator_factor_, 1.0f);
  debugPrint(logger_,
             PAR,
             "partitioning",
             1,
             "Set ILP accelerator factor to {}",
             ilp_accelerator_factor_);
}

void Partitioner::DisableIlpAcceleration()
{
  ilp_accelerator_factor_ = 1.0;
  debugPrint(logger_,
             PAR,
             "partitioning",
             1,
             "Reset ILP accelerator factor to {}",
             ilp_accelerator_factor_);
}

// The main function of Partitioning
void Partitioner::Partition(const HGraphPtr& hgraph,
                            const Matrix<float>& upper_block_balance,
                            const Matrix<float>& lower_block_balance,
                            std::vector<int>& solution,
                            PartitionType partitioner_choice) const
{
  if (static_cast<int>(solution.size()) != hgraph->GetNumVertices()) {
    solution.clear();
    solution.resize(hgraph->GetNumVertices());
    std::fill(solution.begin(), solution.end(), -1);
  }
  switch (partitioner_choice) {
    case PartitionType::INIT_RANDOM:
      RandomPart(hgraph, upper_block_balance, lower_block_balance, solution);
      break;

    case PartitionType::INIT_RANDOM_VILE:
      RandomPart(
          hgraph, upper_block_balance, lower_block_balance, solution, true);
      break;

    case PartitionType::INIT_VILE:
      VilePart(hgraph, solution);
      break;

    case PartitionType::INIT_DIRECT_ILP:
      ILPPart(hgraph, upper_block_balance, lower_block_balance, solution);
      break;

    default:
      RandomPart(hgraph, upper_block_balance, lower_block_balance, solution);
      break;
  }
}

// ------------------------------------------------------------------------------
// The remaining functions are all private functions
// ------------------------------------------------------------------------------
// Different to other random partitioning,
// we enable two modes of random partitioning.
// If vile_mode == false, we try to generate balanced random partitioning
// If vile_mode == true,  we try to generate unbalanced random partitioning

// random partitioning
void Partitioner::RandomPart(const HGraphPtr& hgraph,
                             const Matrix<float>& upper_block_balance,
                             const Matrix<float>& lower_block_balance,
                             std::vector<int>& solution,
                             bool vile_mode) const
{
  // the summation of vertex weights for vertices in current block
  Matrix<float> block_balance(
      num_parts_, std::vector<float>(hgraph->GetVertexDimensions(), 0.0f));
  // determine all the free vertices
  std::vector<bool> visited(hgraph->GetNumVertices(),
                            false);  // if the vertex has been visited
  std::vector<int> vertices;         // the vertices can be moved
  std::vector<int> path_vertices;
  // Step 1: check fixed vertices
  if (hgraph->HasFixedVertices()) {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      if (hgraph->GetFixedAttr(v) > -1) {
        solution[v] = hgraph->GetFixedAttr(v);
        block_balance[solution[v]]
            = block_balance[solution[v]] + hgraph->GetVertexWeights(v);
        visited[v] = true;
      }
    }
  }
  // Step 2: put all the vertices related to critical timing paths together
  if (hgraph->GetNumTimingPaths() > 0) {
    for (int i = 0; i < hgraph->GetNumTimingPaths(); ++i) {
      for (const int v : hgraph->PathVertices(i)) {
        if (visited[v] == false) {
          visited[v] = true;
          path_vertices.push_back(v);
        }
      }  // finish current path
    }    // finish all the paths
    std::shuffle(path_vertices.begin(),
                 path_vertices.end(),
                 std::default_random_engine(seed_));
  }
  // Step 3: check remaining vertices
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    if (visited[v] == false) {
      vertices.push_back(v);
    }
  }
  std::shuffle(
      vertices.begin(), vertices.end(), std::default_random_engine(seed_));
  // Step 4: concatenate path_vertices and vertices
  // Here we insert path_vertices at the beginning,
  // Hopefully we can push all the path_vertices into one block
  vertices.insert(vertices.begin(), path_vertices.begin(), path_vertices.end());

  if (vile_mode == false) {
    // try to generate balanced random partitioning
    int block_id = 0;
    for (const auto& v : vertices) {
      solution[v] = block_id;
      block_balance[block_id]
          = block_balance[block_id] + hgraph->GetVertexWeights(v);
      if (block_balance[block_id] >= lower_block_balance[block_id]) {
        block_id++;
        block_id = block_id % num_parts_;  // adjust the block_id
      }
    }
  } else {
    // try to generate unbalanced random partitioning
    // Basically, for 0, ...., num_parts - 2 blocks, we try to satisfy
    // only lower block balance, then put all remaining vertices to last block
    int block_id = 0;
    bool stop_flag = false;
    for (const auto& v : vertices) {
      solution[v] = block_id;
      block_balance[block_id]
          = block_balance[block_id] + hgraph->GetVertexWeights(v);
      if (block_balance[block_id] >= upper_block_balance[block_id]
          && stop_flag == false) {
        block_id++;
        solution[v] = block_id;  // move the vertex to next block
        if (block_id == num_parts_ - 1) {
          stop_flag = true;
        }
      }
    }
  }
}

// ILP-based partitioning
void Partitioner::ILPPart(const HGraphPtr& hgraph,
                          const Matrix<float>& upper_block_balance,
                          const Matrix<float>& lower_block_balance,
                          std::vector<int>& solution) const
{
  debugPrint(logger_,
             PAR,
             "partitioning",
             1,
             "Starting Optimal ILP-based Partitioning") std::map<int, int>
      fixed_vertices_map;
  Matrix<float> vertex_weights;  // two-dimensional
  vertex_weights.reserve(hgraph->GetNumVertices());
  Matrix<int> hyperedges;                // hyperedges
  std::vector<float> hyperedge_weights;  // one-dimensional
  // set vertices
  for (int v = 0; v < hgraph->GetNumVertices(); v++) {
    vertex_weights.push_back(hgraph->GetVertexWeights(v));
  }
  // check fixed vertices
  if (hgraph->HasFixedVertices()) {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      if (hgraph->GetFixedAttr(v) > -1) {
        solution[v] = hgraph->GetFixedAttr(v);
      }
    }
  }
  // check the hyperedges to be used
  std::vector<int> edge_mask;  // store the hyperedges being used.
  if (ilp_accelerator_factor_ >= 1.0) {
    debugPrint(
        logger_,
        PAR,
        "partitioning",
        1,
        "All the hyperedges will be used in the ILP-based partitioning!");
    edge_mask.resize(hgraph->GetNumHyperedges());
    std::iota(edge_mask.begin(), edge_mask.end(), 0);
  } else if (ilp_accelerator_factor_ > 0.0) {
    // define comp structure to compare hyperedge ( function: >)
    struct comp
    {
      // comparator function
      bool operator()(const std::pair<int, float>& l,
                      const std::pair<int, float>& r) const
      {
        if (l.second != r.second) {
          return l.second > r.second;
        }
        return l.first < r.first;
      }
    };
    // use set data structure to sort hyperedges
    std::set<std::pair<int, float>, comp> unvisited_hyperedges;
    float tot_cost = 0.0;  // total hyperedge cut
    for (auto e = 0; e < hgraph->GetNumHyperedges(); ++e) {
      const float score = evaluator_->CalculateHyperedgeCost(e, hgraph);
      unvisited_hyperedges.insert(std::pair<int, float>(e, score));
      tot_cost += score;
    }
    // pick the top important hyperedges
    tot_cost *= ilp_accelerator_factor_;
    float cur_total_score = 0.0;
    for (auto& value : unvisited_hyperedges) {
      if (cur_total_score <= tot_cost) {
        edge_mask.push_back(value.first);
        cur_total_score += value.second;
      } else {
        break;  // the set has been sorted
      }
    }
    debugPrint(logger_,
               PAR,
               "partitioning",
               1,
               "ilp_accelerator_factor = {}",
               ilp_accelerator_factor_);
    debugPrint(logger_,
               PAR,
               "partitioning",
               1,
               "Reduce the number of hyperedges from {} to {}.",
               hgraph->GetNumHyperedges(),
               edge_mask.size());
  } else {
    debugPrint(logger_,
               PAR,
               "partitioning",
               1,
               "ilp_accelerator_factor = {}",
               ilp_accelerator_factor_);
    debugPrint(logger_, PAR, "partitioning", 1, "No hyperedges will be used");
  }
  // update hyperedges and hyperedge_weights
  hyperedge_weights.reserve(edge_mask.size());
  hyperedges.reserve(edge_mask.size());
  for (auto& e : edge_mask) {
    std::vector<int> hyperedge;
    for (const int vertex_id : hgraph->Vertices(e)) {
      hyperedge.push_back(vertex_id);
    }
    hyperedges.push_back(hyperedge);
    hyperedge_weights.push_back(evaluator_->CalculateHyperedgeCost(e, hgraph));
  }
  if (ILPPartitionInst(num_parts_,
                       hgraph->GetVertexDimensions(),
                       solution,
                       fixed_vertices_map,
                       hyperedges,
                       hyperedge_weights,
                       vertex_weights,
                       upper_block_balance,
                       lower_block_balance)) {
  } else {
    debugPrint(
        logger_,
        PAR,
        "partitioning",
        1,
        "Optimal ILP-based Partitioning failed. Calling random partitioning.");
    RandomPart(hgraph, upper_block_balance, lower_block_balance, solution);
  }
}

// randomly pick one vertex into each block
// then use refinement functions to get valid solution (See Multilevel.cpp)
void Partitioner::VilePart(const HGraphPtr& hgraph,
                           std::vector<int>& solution) const
{
  std::fill(solution.begin(), solution.end(), 0);
  std::vector<int> unvisited;
  unvisited.reserve(hgraph->GetNumVertices());
  // sort the vertices based on vertex weight
  // calculate the weight for all the vertices
  std::vector<float> average_sizes(hgraph->GetNumVertices(), 0.0);
  // check fixed vertices
  // Step 1: check fixed vertices
  if (hgraph->HasFixedVertices()) {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      if (hgraph->GetFixedAttr(v) > -1) {
        solution[v] = hgraph->GetFixedAttr(v);
      } else {
        unvisited.push_back(v);
        average_sizes[v] = evaluator_->GetVertexWeightNorm(v, hgraph);
      }
    }
  } else {
    for (int v = 0; v < hgraph->GetNumVertices(); v++) {
      unvisited.push_back(v);
      average_sizes[v] = evaluator_->GetVertexWeightNorm(v, hgraph);
    }
  }
  // Step 2: sort remaining vertices based on vertex
  // define the sort function
  auto lambda_sort_criteria = [&](int& x, int& y) -> bool {
    return average_sizes[x] < average_sizes[y];
  };
  std::sort(unvisited.begin(), unvisited.end(), lambda_sort_criteria);
  // Evenly distribute all the vertices into blocks based on sorted order
  int block_id = 0;
  for (const auto& v : unvisited) {
    solution[v] = block_id;
    block_id++;
    block_id = block_id % num_parts_;  // adjust the block_id
  }
}

}  // namespace par

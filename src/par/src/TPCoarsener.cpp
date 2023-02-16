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
// High-level description
// This file includes the implementation for first-choice based coarsening
// Coarsening is an operation class:  It takes a hypergraph hgraph and returns
// a sequence of coarser hypergraphs. (Top level function: LazyFirstChoice)
///////////////////////////////////////////////////////////////////////////////
#include "TPCoarsener.h"

#include <cassert>
#include <set>

#include "TPHypergraph.h"
#include "Utilities.h"
#include "utl/Logger.h"

using utl::PAR;

// PAR 2504
namespace par {

void TPcoarsener::OrderVertices(const HGraph hgraph, std::vector<int>& vertices)
{
  int order_choice = GetVertexOrderChoice();
  if (order_choice == RANDOM) {
    shuffle(
        vertices.begin(), vertices.end(), std::default_random_engine(seed_));
  } else if (order_choice == DEGREE) {
    std::vector<int> degrees(hgraph->num_vertices_, 0);
    for (int i = 0; i < vertices.size(); ++i) {
      const int vertex = vertices[i];
      const int first_valid_entry_h = hgraph->vptr_[vertex];
      const int first_invalid_entry_h = hgraph->vptr_[vertex + 1];
      for (int j = first_valid_entry_h; j < first_invalid_entry_h; ++j) {
        const int he = hgraph->vind_[j];
        const int first_valid_entry_v = hgraph->eptr_[he];
        const int first_invalid_entry_k = hgraph->eptr_[he + 1];
        for (int k = first_valid_entry_v; k < first_invalid_entry_k; ++k) {
          const int nbr = hgraph->eind_[k];
          if (nbr == vertex) {
            continue;
          }
          ++degrees[vertex];
        }
      }
    }
    auto lambda_sort_criteria
        = [&](int& x, int& y) -> bool { return degrees[x] < degrees[y]; };
    std::sort(vertices.begin(), vertices.end(), lambda_sort_criteria);

  } else if (order_choice == SIZE) {
    std::vector<float> average_sizes(hgraph->num_vertices_, 0.0);
    for (int i = 0; i < vertices.size(); ++i) {
      const int vertex = vertices[i];
      std::vector<float> wt = hgraph->vertex_weights_[vertex];
      float avg_wt = std::accumulate(wt.begin(), wt.end(), 0.0) / wt.size();
      average_sizes[vertex] = avg_wt;
    }
    auto lambda_sort_criteria = [&](int& x, int& y) -> bool {
      return average_sizes[x] < average_sizes[y];
    };
    std::sort(vertices.begin(), vertices.end(), lambda_sort_criteria);
  } else if (order_choice == DEFAULT) {
    // no shuffling just stick with default ordering of vertices
  } else if (order_choice == SPECTRAL) {
    // need code here for spectral based ordering
  } else if (order_choice == TIMING) {
    // need code here for timing paths based vertex ordering
    /*std::set<int> path_vertices;
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
    shuffle(
        unvisited.begin(), unvisited.end(), std::default_random_engine(seed_));
    unvisited.insert(
        unvisited.begin(), path_vertices.begin(), path_vertices.end());*/
  }
}

inline const float TPcoarsener::GetClusterScore(
    const int he,
    HGraph hgraph,
    std::vector<float>& algebraic_weights)
{
  const int he_size = hgraph->eptr_[he + 1] - hgraph->eptr_[he];
  const float he_score
      = std::inner_product(hgraph->hyperedge_weights_[he].begin(),
                           hgraph->hyperedge_weights_[he].end(),
                           e_wt_factors_.begin(),
                           0.0)
        / (he_size - 1) * algebraic_weights[he];
  return he_score;
}

inline const std::vector<float> TPcoarsener::AverageClusterWt(
    const HGraph hgraph)
{
  std::vector<float> total_wt(hgraph->vertex_dimensions_, 0.0);
  for (int i = 0; i < hgraph->num_vertices_; ++i) {
    total_wt = total_wt + hgraph->vertex_weights_[i];
  }
  float scale = 4.5;
  return DivideFactor(total_wt, hgraph->num_vertices_ / scale);
}

void TPcoarsener::VertexMatching(
    const HGraph hgraph,
    std::vector<int>& vertex_c_attr,
    std::vector<std::vector<float>>& vertex_weights_c,
    std::vector<int>& community_attr_c,
    std::vector<int>& fixed_attr_c,
    std::vector<std::vector<float>>& placement_attr_c)
{
  int num_clusters_est = hgraph->num_vertices_;
  int cluster_id = 0;
  vertex_c_attr.clear();
  vertex_c_attr.resize(hgraph->num_vertices_);
  std::fill(vertex_c_attr.begin(), vertex_c_attr.end(), -1);
  vertex_weights_c.clear();
  placement_attr_c.clear();
  community_attr_c.clear();
  fixed_attr_c.clear();
  std::vector<float> algebraic_weights = hgraph->ComputeAlgebraicWights();
  std::vector<int> unvisited;
  // Collect unvisited vertices at beginning of coarsening
  // Ensure that fixed vertices in the hypergraph are not touched
  if (hgraph->fixed_vertex_flag_ == false) {
    unvisited.resize(hgraph->num_vertices_);
    std::iota(unvisited.begin(), unvisited.end(), 0);
  } else {
    for (int v = 0; v < hgraph->num_vertices_; ++v) {
      if (hgraph->fixed_attr_[v] > -1) {
        vertex_c_attr[v] = cluster_id++;
        vertex_weights_c.push_back(hgraph->vertex_weights_[v]);
        fixed_attr_c.push_back(hgraph->fixed_attr_[v]);
        if (hgraph->community_flag_ == true) {
          community_attr_c.push_back(hgraph->community_attr_[v]);
        }
        if (hgraph->placement_flag_ == true) {
          placement_attr_c.push_back(hgraph->placement_attr_[v]);
        }
      } else {
        unvisited.push_back(v);
      }
    }
  }
  OrderVertices(hgraph, unvisited);
  // thr_cluster_weight_ = AverageClusterWt(hgraph);
  for (auto v_itr = unvisited.begin(); v_itr != unvisited.end(); ++v_itr) {
    const int v = *v_itr;
    if (vertex_c_attr[v] != -1) {
      continue;
    }
    std::map<int, float> score_map;
    const int first_valid_entry_v = hgraph->vptr_[v];
    const int first_invalid_entry_v = hgraph->vptr_[v + 1];
    int he_reduction_count = 0;
    for (int i = first_valid_entry_v; i < first_invalid_entry_v; ++i) {
      const int he = hgraph->vind_[i];
      const int first_valid_entry_he = hgraph->eptr_[he];
      const int first_invalid_entry_he = hgraph->eptr_[he + 1];
      const int he_size = first_invalid_entry_he - first_valid_entry_he;
      if (he_size <= 1 || he_size > thr_coarsen_hyperedge_size_) {
        continue;
      }
      // Count how many 2 pin hyperedges get removed by this cluster
      if (he_size == 2) {
        ++he_reduction_count;
      }
      const float he_score = GetClusterScore(he, hgraph, algebraic_weights);
      for (int j = first_valid_entry_he; j < first_invalid_entry_he; ++j) {
        const int nbr_v = hgraph->eind_[j];
        if (nbr_v == v) {
          continue;
        }
        if (score_map.find(nbr_v) == score_map.end()) {
          if ((hgraph->fixed_vertex_flag_ == true
               && hgraph->fixed_attr_[nbr_v] > -1)
              || (hgraph->community_flag_ == true
                  && hgraph->community_attr_[nbr_v]
                         != hgraph->community_attr_[v])) {
            continue;
          }
          const std::vector<float>& nbvr_v_weight
              = vertex_c_attr[nbr_v] > -1
                    ? vertex_weights_c[vertex_c_attr[nbr_v]]
                    : hgraph->vertex_weights_[nbr_v];
          if (hgraph->vertex_weights_[v] + nbvr_v_weight
              > thr_cluster_weight_) {
            continue;
          }
          score_map[nbr_v] = he_score;
        } else {
          score_map[nbr_v] += he_score;
        }
        // score_map[nbr_v] += 0.25 * he_reduction_count;
      }
    }

    if (score_map.size() == 0) {
      vertex_c_attr[v] = cluster_id++;
      vertex_weights_c.push_back(hgraph->vertex_weights_[v]);
      if (hgraph->placement_flag_ == true) {
        placement_attr_c.push_back(hgraph->placement_attr_[v]);
      }
      if (hgraph->community_flag_ == true) {
        community_attr_c.push_back(hgraph->community_attr_[v]);
      }
      if (hgraph->fixed_vertex_flag_ == true) {
        fixed_attr_c.push_back(-1);
      }
      continue;
    }
    if (hgraph->num_timing_paths_ > 0 && path_traverse_step_ > 0) {
      const int first_valid_entry_pv = hgraph->pptr_v_[v];
      const int first_invalid_entry_pv = hgraph->pptr_v_[v + 1];
      std::map<int, float> timing_neighbors;
      for (int j = first_valid_entry_pv; j < first_invalid_entry_pv; ++j) {
        const int p = hgraph->pind_v_[j];
        const int first_valid_entry_p = hgraph->vptr_p_[p];
        const int first_invalid_entry_p = hgraph->vptr_p_[p + 1];
        const float timing_attr
            = hgraph->timing_attr_[p] * timing_factor_
              / path_traverse_step_;  // path_traverse_step is for normalizing
        int p_ptr = hgraph->vptr_p_[p];
        while (p_ptr < first_invalid_entry_p) {
          if (hgraph->vind_p_[p_ptr] == v) {
            auto TraversePath = [&](bool direction) {
              int i = 0;
              while (i++ < path_traverse_step_) {
                const int idx = (direction == true) ? p_ptr + i : p_ptr - i;
                if (idx < first_valid_entry_p || idx >= first_invalid_entry_p) {
                  return;
                }
                const int u = hgraph->vind_p_[idx];
                if (u == v) {
                  return;
                }
                if (timing_neighbors.find(u) == timing_neighbors.end()) {
                  timing_neighbors[u] = i * timing_attr;
                } else {
                  timing_neighbors[u] += i * timing_attr;
                }
              }
            };
            TraversePath(true);
            TraversePath(false);
          }
          p_ptr++;
        }
      }
      for (auto& [u, score] : timing_neighbors) {
        if (score_map.find(u) != score_map.end()) {
          score_map[u] += score;
        }
      }
    }
    float best_score = -std::numeric_limits<float>::max();
    int best_candidate = -1;
    for (auto& [u, score] : score_map) {
      if (hgraph->placement_flag_ == true) {
        const std::vector<float>& u_placement
            = vertex_c_attr[u] > -1 ? placement_attr_c[vertex_c_attr[u]]
                                    : hgraph->placement_attr_[u];
        score += norm2(u_placement - hgraph->placement_attr_[v], p_wt_factors_);
      }
      if ((score > best_score)
          || (score == best_score && vertex_c_attr[u] == -1)) {
        best_candidate = u;
        best_score = score;
      }
    }
    const int best_candidate_c_attr = vertex_c_attr.at(best_candidate);
    if (best_candidate_c_attr > -1) {
      vertex_c_attr[v] = best_candidate_c_attr;
      if (hgraph->placement_flag_ == true) {
        const float w1
            = norm2(vertex_weights_c[best_candidate_c_attr], v_wt_factors_);
        const float w2 = norm2(hgraph->vertex_weights_[v], v_wt_factors_);
        placement_attr_c[best_candidate_c_attr]
            = placement_attr_c[best_candidate_c_attr] * (w1 / (w1 + w2))
              + hgraph->placement_attr_[v] * (w2 / (w1 + w2));
      }
      vertex_weights_c[best_candidate_c_attr]
          = vertex_weights_c[best_candidate_c_attr]
            + hgraph->vertex_weights_[v];
    } else {
      vertex_c_attr[v] = cluster_id;
      vertex_c_attr[best_candidate] = cluster_id++;
      if (hgraph->placement_flag_ == true) {
        const float w1
            = norm2(hgraph->vertex_weights_[best_candidate], v_wt_factors_);
        const float w2 = norm2(hgraph->vertex_weights_[v], v_wt_factors_);
        placement_attr_c.push_back(
            hgraph->placement_attr_[best_candidate] * (w1 / (w1 + w2))
            + hgraph->placement_attr_[v] * (w2 / (w1 + w2)));
      }
      vertex_weights_c.push_back(hgraph->vertex_weights_[best_candidate]
                                 + hgraph->vertex_weights_[v]);
      if (hgraph->community_flag_ == true) {
        community_attr_c.push_back(hgraph->community_attr_[best_candidate]);
      }
      if (hgraph->fixed_vertex_flag_ == true) {
        fixed_attr_c.push_back(-1);
      }
    }
    --num_clusters_est;
    const float current_coarsen_ratio
        = static_cast<float>(hgraph->num_vertices_)
          / static_cast<float>(num_clusters_est);
    if (current_coarsen_ratio > coarsen_ratio_) {
      while (v_itr != unvisited.end()) {
        const int& v = *v_itr++;
        if (vertex_c_attr[v] == -1) {
          vertex_c_attr[v] = cluster_id++;
          vertex_weights_c.push_back(hgraph->vertex_weights_[v]);
          if (hgraph->fixed_vertex_flag_ == true) {
            fixed_attr_c.push_back(-1);
          }
          if (hgraph->placement_flag_ == true) {
            placement_attr_c.push_back(hgraph->placement_attr_[v]);
          }
          if (hgraph->community_flag_ == true) {
            community_attr_c.push_back(hgraph->community_attr_[v]);
          }
        }
      }
      return;
    }
  }
}

HGraph TPcoarsener::Contraction(HGraph hgraph,
                                std::vector<int>& vertex_c_attr,
                                std::vector<int>& community_attr_c,
                                std::vector<int>& fixed_attr_c,
                                TP_matrix<float>& vertex_weights_c,
                                TP_matrix<float>& placement_attr_c)
{
  TP_matrix<int> hyperedges_c;
  TP_matrix<float> hyperedges_weights_c;
  TP_matrix<float> nonscaled_hyperedges_weights_c;
  std::map<long long int, int> hash_map;
  for (int i = 0; i < hgraph->num_hyperedges_; ++i) {
    const int first_valid_entry = hgraph->eptr_[i];
    const int first_invalid_entry = hgraph->eptr_[i + 1];
    const int he_size = first_invalid_entry - first_valid_entry;
    if (he_size <= 1 || he_size > thr_match_hyperedge_size_) {
      continue;
    }
    std::set<int> hyperedge_c;
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      hyperedge_c.insert(vertex_c_attr[hgraph->eind_[j]]);
    }
    if (hyperedge_c.size() <= 1) {
      continue;
    }
    const long long int hash_value
        = std::inner_product(hyperedge_c.begin(),
                             hyperedge_c.end(),
                             hyperedge_c.begin(),
                             static_cast<long long int>(0));
    if (hash_map.find(hash_value) == hash_map.end()) {
      hash_map[hash_value] = static_cast<int>(hyperedges_c.size());
      hyperedges_weights_c.push_back(hgraph->hyperedge_weights_[i]);
      if (hgraph->num_timing_paths_ > 0) {
        nonscaled_hyperedges_weights_c.push_back(
            hgraph->nonscaled_hyperedge_weights_[i]);
      }
      hyperedges_c.push_back(
          std::vector<int>(hyperedge_c.begin(), hyperedge_c.end()));
    } else {
      const int hash_id = hash_map[hash_value];
      std::vector<int> hyperedge_vec(hyperedge_c.begin(), hyperedge_c.end());
      if (hyperedges_c[hash_id] == hyperedge_vec) {
        hyperedges_weights_c[hash_id]
            = hyperedges_weights_c[hash_id] + hgraph->hyperedge_weights_[i];
        if (hgraph->num_timing_paths_ > 0) {
          nonscaled_hyperedges_weights_c[hash_id]
              = nonscaled_hyperedges_weights_c[hash_id]
                + hgraph->nonscaled_hyperedge_weights_[i];
        }
      } else {
        hyperedges_weights_c.push_back(hgraph->hyperedge_weights_[i]);
        if (hgraph->num_timing_paths_ > 0) {
          nonscaled_hyperedges_weights_c.push_back(
              hgraph->nonscaled_hyperedge_weights_[i]);
        }
        hyperedges_c.push_back(hyperedge_vec);
      }
    }
  }

  TP_matrix<int> paths_c;
  std::vector<float> timing_attr_c;
  hash_map.clear();
  if (hgraph->num_timing_paths_ > 0) {
    for (int p = 0; p < hgraph->num_timing_paths_; ++p) {
      const int first_valid_entry_p = hgraph->vptr_p_[p];
      const int first_invalid_entry_p = hgraph->vptr_p_[p + 1];
      if (first_invalid_entry_p - first_valid_entry_p <= 1) {
        continue;
      }
      std::vector<int> path{
          vertex_c_attr[hgraph->vind_p_[first_valid_entry_p]]};
      for (int i = first_valid_entry_p + 1; i < first_invalid_entry_p; ++i) {
        if (path.back() != hgraph->vind_p_[i]) {
          path.push_back(vertex_c_attr[hgraph->vind_p_[i]]);
        }
      }
      if (path.size() <= 1) {
        continue;
      }
      const long long int hash_value
          = std::inner_product(path.begin(),
                               path.end(),
                               path.begin(),
                               static_cast<long long int>(0));
      if (hash_map.find(hash_value) == hash_map.end()) {
        hash_map[hash_value] = static_cast<int>(paths_c.size());
        paths_c.push_back(path);
        timing_attr_c.push_back(hgraph->timing_attr_[p]);
      } else {
        const int hash_id = hash_map[hash_value];
        if (paths_c[hash_id] == path) {
          timing_attr_c[hash_id]
              = timing_attr_c[hash_id] + hgraph->timing_attr_[p];
        } else {
          paths_c.push_back(path);
          timing_attr_c.push_back(hgraph->timing_attr_[p]);
        }
      }
    }
  }
  hgraph->vertex_c_attr_ = vertex_c_attr;
  return std::make_shared<TPHypergraph>(hgraph->vertex_dimensions_,
                                        hgraph->hyperedge_dimensions_,
                                        hyperedges_c,
                                        vertex_weights_c,
                                        hyperedges_weights_c,
                                        nonscaled_hyperedges_weights_c,
                                        fixed_attr_c,
                                        community_attr_c,
                                        hgraph->placement_dimensions_,
                                        placement_attr_c,
                                        paths_c,
                                        timing_attr_c,
                                        logger_);
}

HGraph TPcoarsener::Aggregate(HGraph hgraph)
{
  std::vector<int> vertex_c_attr;
  std::vector<std::vector<float>> vertex_weights_c;
  std::vector<int> community_attr_c;
  std::vector<int> fixed_attr_c;
  std::vector<std::vector<float>> placement_attr_c;
  VertexMatching(hgraph,
                 vertex_c_attr,
                 vertex_weights_c,
                 community_attr_c,
                 fixed_attr_c,
                 placement_attr_c);
  auto clustered_hgraph = Contraction(hgraph,
                                      vertex_c_attr,
                                      community_attr_c,
                                      fixed_attr_c,
                                      vertex_weights_c,
                                      placement_attr_c);
  // hgraph->vertex_c_attr_ = vertex_c_attr;  // update clustering attr
  return clustered_hgraph;
}

std::vector<int> TPcoarsener::PathBasedCommunity(HGraph hgraph)
{
  std::set<int> path_community;
  for (int i = 0; i < hgraph->num_timing_paths_; ++i) {
    const int first_valid_entry = hgraph->vptr_p_[i];
    const int first_invalid_entry = hgraph->vptr_p_[i + 1];
    for (int j = first_valid_entry; j < first_invalid_entry; ++j) {
      int v = hgraph->vind_p_[j];
      path_community.insert(v);
    }
  }
  std::vector<int> community(path_community.begin(), path_community.end());
  std::vector<int> vtx_community(hgraph->num_vertices_, 0);
  for (auto& v : community) {
    vtx_community[v] = 1;
  }
  return vtx_community;
}

/*
// Future faster implementation of FC

int TPcoarsener::FirstChoice(int num_vertices,
                             int num_hyperedges,
                             std::vector<int> adj_list,
                             std::vector<float> wt_list,
                             std::vector<int> coarsened_hypergraph)
{
  std::vector<int> degree(num_vertices, 0);
  std::vector<int> matching(num_vertices, -1);
  std::vector<int> merge_stack(num_vertices, 0);
  int num_coarsened_vertices = 0;

  // Initialize degree array
  for (int i = 0; i < num_vertices; ++i) {
    degree[i] = 0;
    for (int j = adj_list[i]; j < adj_list[i + 1]; ++j) {
      degree[i] += wt_list[j];  // adding wts
    }
  }

  // Repeat untill all vertices are matched
  for (int i = 0; i < num_vertices; ++i) {
    if (matching[i] == -1) {
      int top = 0;
      merge_stack[top] = i;
      while (top >= 0) {
        int u = merge_stack[top];
        --top;
        for (int j = adj_list[u]; j < adj_list[u + 1]; ++j) {
          int v = adj_list[j];
          if (matching[v] == -1) {
            matching[v] = u;
            merge_stack[++top] = v;
            break;
          } else if (degree[u] + degree[v]
                     > degree[matching[u]] + degree[matching[v]]) {
            int w = matching[v];
            matching[v] = u;
            merge_stack[++top] = w;
          }
        }
      }
    }
  }

  // Form coarser hypergraph

  for (int i = 0; i < num_vertices; ++i) {
    if (matching[i] >= i) {
      coarsened_hypergraph[num_coarsened_vertices++] = i;
    }
  }
}
*/

TP_coarse_graphs TPcoarsener::LazyFirstChoice(HGraph hgraph)
{
  const auto start_timestamp = std::chrono::high_resolution_clock::now();
  std::vector<HGraph> hierarchy;  // a sequence of coarser hypergraphs
  hierarchy.push_back(hgraph);    // push original hgraph to vector
  int cur_num_vertices = hgraph->num_vertices_;
  logger_->report("=========================================");
  logger_->report("[STATUS] Running FC multilevel coarsening ");
  logger_->report("=========================================");
  logger_->report(
      "[COARSEN] Level 0 :: {}, {}, {}, {}",
      hierarchy.back()->num_vertices_,
      hierarchy.back()->num_hyperedges_,
      GetVectorString(hierarchy.back()->GetTotalVertexWeights()),
      GetVectorString(hierarchy.back()->GetTotalHyperedgeWeights()));
  for (int num_iter = 0; num_iter < thr_coarsen_iters_; ++num_iter) {
    // do coarsening step
    int num_vertices_old = hierarchy.back()->num_vertices_;
    auto hg = Aggregate(hierarchy.back());
    int num_vertices_new = hg->num_vertices_;
    if (num_vertices_new == num_vertices_old) {
      break;
    }
    hierarchy.push_back(hg);
    logger_->report(
        "[COARSEN] Level {} :: {}, {}, {}, {}",
        num_iter + 1,
        hierarchy.back()->num_vertices_,
        hierarchy.back()->num_hyperedges_,
        GetVectorString(hierarchy.back()->GetTotalVertexWeights()),
        GetVectorString(hierarchy.back()->GetTotalHyperedgeWeights()));
    const float cut_diff_ratio
        = static_cast<float>(cur_num_vertices - hierarchy.back()->num_vertices_)
          / static_cast<float>(hierarchy.back()->num_vertices_);
    // check the difference between adjacent hypergraphs
    if (cut_diff_ratio <= adj_diff_ratio_)
      break;
    else
      cur_num_vertices = hierarchy.back()->num_vertices_;

    // check early stop conditions
    if (hierarchy.back()->num_vertices_ <= thr_coarsen_vertices_
        || hierarchy.back()->num_hyperedges_ <= thr_coarsen_hyperedges_) {
      break;
    }
  }
  auto end_timestamp = std::chrono::high_resolution_clock::now();
  double time_taken = std::chrono::duration_cast<std::chrono::nanoseconds>(
                          end_timestamp - start_timestamp)
                          .count()
                      * 1e-9;
  logger_->info(PAR, 1, "Hierarchical coarsening time {} seconds", time_taken);
  return hierarchy;
}
}  // namespace par

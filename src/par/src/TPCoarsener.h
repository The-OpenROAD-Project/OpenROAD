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
#pragma once
//
// This file contains the classes for Coarsening Phase
// Coarsening Phase is to generate a sequence of coarser hypergraph
// We define Coarsening as an operator class.
// It will accept a HGraph (std::shared_ptr<TPHypergraph>) as input
// and return a sequence of coarser hypergraphs
//
#include "TPHypergraph.h"
#include "utl/Logger.h"

namespace par {
template <typename T>
using TP_matrix = std::vector<std::vector<T>>;

using TP_coarse_graphs = std::vector<HGraph>;

enum order
{
  RANDOM,
  DEGREE,
  SIZE,
  DEFAULT,
  SPECTRAL,
  TIMING
};

class TPcoarsener
{
 public:
  TPcoarsener() = default;
  TPcoarsener(const std::vector<float> e_wt_factors,
              const std::vector<float> v_wt_factors,
              const std::vector<float> p_wt_factors,
              const float timing_factor,
              const int path_traverse_step,
              const std::vector<float> thr_cluster_weight,
              const int thr_coarsen_hyperedge_size,
              const int thr_match_hyperedge_size,
              const int thr_coarsen_vertices,
              const int thr_coarsen_hyperedges,
              const float coarsen_ratio,
              const int thr_coarsen_iters,
              const float adj_diff_ratio,
              const int seed,
              utl::Logger* logger)
      : e_wt_factors_(e_wt_factors),
        v_wt_factors_(v_wt_factors),
        p_wt_factors_(p_wt_factors),
        timing_factor_(timing_factor),
        path_traverse_step_(path_traverse_step),
        thr_cluster_weight_(thr_cluster_weight),
        thr_coarsen_hyperedge_size_(thr_coarsen_hyperedge_size),
        thr_match_hyperedge_size_(thr_match_hyperedge_size),
        thr_coarsen_vertices_(thr_coarsen_vertices),
        thr_coarsen_hyperedges_(thr_coarsen_hyperedges),
        coarsen_ratio_(coarsen_ratio),
        thr_coarsen_iters_(thr_coarsen_iters),
        adj_diff_ratio_(adj_diff_ratio),
        seed_(seed),
        logger_(logger)
  {
  }
  TPcoarsener(const TPcoarsener&) = default;
  TPcoarsener(TPcoarsener&&) = default;
  TPcoarsener& operator=(const TPcoarsener&) = default;
  TPcoarsener& operator=(TPcoarsener&&) = default;
  ~TPcoarsener() = default;
  void SetVertexOrderChoice(const int choice) { vertex_order_choice_ = choice; }
  int GetVertexOrderChoice() const { return vertex_order_choice_; }
  std::vector<int> PathBasedCommunity(HGraph hgraph);
  TP_coarse_graphs LazyFirstChoice(HGraph hgraph);

 private:
  inline const std::vector<float> AverageClusterWt(const HGraph hgraph);
  void OrderVertices(const HGraph hgraph, std::vector<int>& vertices);
  inline const float GetClusterScore(const int he,
                                     HGraph hgraph,
                                     std::vector<float>& algebraic_weights);
  HGraph Contraction(HGraph hgraph,
                     std::vector<int>& vertex_c_atrr,
                     std::vector<int>& community_attr_c,
                     std::vector<int>& fixed_attr_c,
                     TP_matrix<float>& vertex_weights_c,
                     TP_matrix<float>& placement_attr_c);
  HGraph Aggregate(HGraph hgraph);
  void VertexMatching(const HGraph hgraph,
                      std::vector<int>& vertex_c_attr,
                      std::vector<std::vector<float>>& vertex_weights_c,
                      std::vector<int>& community_attr_c,
                      std::vector<int>& fixed_attr_c,
                      std::vector<std::vector<float>>& placement_attr_c);
  std::vector<float> e_wt_factors_;
  std::vector<float> v_wt_factors_;
  std::vector<float> p_wt_factors_;
  float timing_factor_;
  int path_traverse_step_;
  std::vector<float> thr_cluster_weight_;
  int thr_coarsen_hyperedge_size_;
  int thr_match_hyperedge_size_;
  int thr_coarsen_vertices_;
  int thr_coarsen_hyperedges_;
  float coarsen_ratio_;
  int thr_coarsen_iters_;
  float adj_diff_ratio_;
  int seed_;
  int vertex_order_choice_;
  utl::Logger* logger_ = nullptr;
  friend class MultiLevelHierarchy;
};
// nickname for shared pointer of Coarsening
using TP_coarsening_ptr = std::shared_ptr<TPcoarsener>;
}  // namespace par

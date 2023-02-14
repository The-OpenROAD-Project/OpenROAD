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

#include "TPCoarsener.h"
#include "TPHypergraph.h"
#include "TPPartitioner.h"
#include "TPRefiner.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

enum RefinerType
{
  KFM_REFINEMENT,  // direct k-way FM refinement
  KPM_REFINEMENT,  // pair-wise k-way FM refinement
  KILP_REFINEMENT  // pair-wise ILP-based refinement
};

using TP_partition = std::vector<int>;

template <typename T>
using matrix = std::vector<std::vector<T>>;

class TPmultilevelPartitioner
{
 public:
  TPmultilevelPartitioner() = default;
  TPmultilevelPartitioner(
      TP_coarsening_ptr coarsener,
      TP_partitioning_ptr partitioner,
      TP_two_way_refining_ptr two_way_refiner,
      TP_greedy_refiner_ptr greedy_refiner,
      TP_ilp_refiner_ptr ilp_refiner,
      int num_parts,
      bool v_cycle_flag,
      int num_initial_solutions,       // number of initial random solutions
      int num_best_initial_solutions,  // number of best initial solutions
      int num_ubfactor_delta,   // allowing marginal imbalance to improve QoR
      int max_num_vcycle,       // maximum number of vcycles
      int seed,                 // random seed
      float ub_factor,          // ubfactor
      RefinerType refine_type,  // refinement type
      utl::Logger* logger)
      : v_cycle_flag_(v_cycle_flag),
        coarsener_(coarsener),
        partitioner_(partitioner),
        two_way_refiner_(two_way_refiner),
        greedy_refiner_(greedy_refiner),
        ilp_refiner_(ilp_refiner),
        logger_(logger),
        num_parts_(num_parts),
        num_initial_solutions_(num_initial_solutions),
        num_best_initial_solutions_(num_best_initial_solutions),
        num_ubfactor_delta_(num_ubfactor_delta),
        max_num_vcycle_(max_num_vcycle),
        seed_(seed),
        ub_factor_(ub_factor),
        refine_type_(refine_type)
  {
  }
  TPmultilevelPartitioner(
      TP_coarsening_ptr coarsener,
      TP_partitioning_ptr partitioner,
      TP_k_way_refining_ptr k_way_refiner,
      int num_parts,
      bool v_cycle_flag,
      int num_initial_solutions,       // number of initial random solutions
      int num_best_initial_solutions,  // number of best initial solutions
      int num_ubfactor_delta,   // allowing marginal imbalance to improve QoR
      int max_num_vcycle,       // maximum number of vcycles
      int seed,                 // random seed
      float ub_factor,          // ubfactor
      RefinerType refine_type,  // refinement type
      utl::Logger* logger)
      : v_cycle_flag_(v_cycle_flag),
        coarsener_(coarsener),
        partitioner_(partitioner),
        k_way_refiner_(k_way_refiner),
        logger_(logger),
        num_parts_(num_parts),
        num_initial_solutions_(num_initial_solutions),
        num_best_initial_solutions_(num_best_initial_solutions),
        num_ubfactor_delta_(num_ubfactor_delta),
        max_num_vcycle_(max_num_vcycle),
        seed_(seed),
        ub_factor_(ub_factor),
        refine_type_(refine_type)
  {
  }
  TPmultilevelPartitioner(const TPmultilevelPartitioner&) = default;
  TPmultilevelPartitioner(TPmultilevelPartitioner&&) = default;
  TPmultilevelPartitioner& operator=(const TPmultilevelPartitioner&) = default;
  TPmultilevelPartitioner& operator=(TPmultilevelPartitioner&&) = default;
  ~TPmultilevelPartitioner() = default;
  int GetBestInitSolns() const { return num_best_initial_solutions_; }
  TP_partition PartitionTwoWay(HGraph hgraph,
                               HGraph hgraph_processed,
                               matrix<float> max_vertex_balance,
                               bool VCycle);
  TP_partition PartitionKWay(HGraph hgraph,
                             HGraph hgraph_processed,
                             matrix<float> max_vertex_balance,
                             bool VCycle);

 private:
  bool v_cycle_flag_;
  TP_coarsening_ptr coarsener_ = nullptr;
  TP_partitioning_ptr partitioner_ = nullptr;
  TP_two_way_refining_ptr two_way_refiner_ = nullptr;
  TP_k_way_refining_ptr k_way_refiner_ = nullptr;
  TP_greedy_refiner_ptr greedy_refiner_ = nullptr;
  TP_ilp_refiner_ptr ilp_refiner_ = nullptr;
  utl::Logger* logger_ = nullptr;
  int num_parts_;
  int num_initial_solutions_;
  int num_best_initial_solutions_;
  int num_ubfactor_delta_;
  int max_num_vcycle_;
  int seed_;
  float ub_factor_;
  RefinerType refine_type_;
  std::vector<int> MapClusters(TP_coarse_graphs hierarchy);
  void ParallelPart(HGraph coarsest_hgraph,
                    matrix<float>& max_vertex_balance,
                    float& cutsize_vec,
                    std::vector<int>& solution,
                    const int seed);
  std::pair<matrix<int>, std::vector<float>> InitialPartTwoWay(
      HGraph coarsest_hypergraph,
      matrix<float>& max_vertex_balance,
      TP_partition& solution);
  void MultilevelPartTwoWay(HGraph hgraph,
                            HGraph hgraph_processed,
                            matrix<float>& max_vertex_balance,
                            TP_partition& solution,
                            bool VCycle);
  void VcycleTwoWay(std::vector<HGraph> hgraph_vec,
                    matrix<float>& max_vertex_balance,
                    TP_partition& solution,
                    TP_two_way_refining_ptr refiner,
                    TP_ilp_refiner_ptr i_refiner,
                    bool print = true);
  void GuidedVcycleTwoWay(TP_partition& solution,
                          HGraph hgraph,
                          matrix<float>& max_vertex_balance,
                          TP_two_way_refining_ptr refiner,
                          TP_ilp_refiner_ptr i_refiner);
  // Functions for K way
  std::pair<matrix<int>, std::vector<int>> InitialPartKWay(
      HGraph coarsest_hypergraph,
      matrix<float>& max_vertex_balance,
      TP_partition& solution);
  void MultilevelPartKWay(HGraph hgraph,
                          HGraph hgraph_processed,
                          matrix<float>& max_vertex_balance,
                          TP_partition& solution,
                          bool VCycle);
  void VcycleKWay(std::vector<HGraph> hgraph_vec,
                  matrix<float>& max_vertex_balance,
                  TP_partition& solution,
                  TP_k_way_refining_ptr refiner,
                  bool print = true);
  void GuidedVcycleKWay(TP_partition& solution,
                        HGraph hgraph,
                        matrix<float>& max_vertex_balance,
                        TP_k_way_refining_ptr refiner);
};

using TP_mlevel_partitioning_ptr = std::shared_ptr<TPmultilevelPartitioner>;

}  // namespace par

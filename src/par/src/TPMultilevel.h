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
#include "TPEvaluator.h"
#include "TPHypergraph.h"
#include "TPPartitioner.h"
#include "TPRefiner.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

using TP_partition = std::vector<int>;

template <typename T>
using matrix = std::vector<std::vector<T> >;

// Multilevel partitioner
class TPmultilevelPartitioner;
using TP_multi_level_partitioner = std::shared_ptr<TPmultilevelPartitioner>;

class TPmultilevelPartitioner
{
 public:
  TPmultilevelPartitioner(const int num_parts,
                          const float ub_factor,
                          // user-specified parameters
                          const bool v_cycle_flag,
                          const int num_initial_solutions,
                          const int num_best_initial_solutions,
                          const int num_ubfactor_delta,
                          const int max_num_vcycle,
                          const int seed,
                          const bool timing_driven_flag, 
                          // pointers
                          TP_coarsening_ptr coarsener,
                          TP_partitioning_ptr partitioner,
                          TP_k_way_refiner_ptr k_way_refiner,
                          TP_k_way_pm_refiner_ptr k_way_pm_refiner,
                          TP_greedy_refiner_ptr greedy_refiner,
                          TP_ilp_refiner_ptr ilp_refiner,
                          TP_evaluator_ptr evaluator,
                          utl::Logger* logger)
    :  num_parts_(num_parts),
       ub_factor_(ub_factor),
       v_cycle_flag_(v_cycle_flag),
       num_initial_solutions_(num_initial_solutions),
       num_best_initial_solutions_(num_best_initial_solutions),
       num_ubfactor_delta_(num_ubfactor_delta),
       max_num_vcycle_(max_num_vcycle),
       seed_(seed),
       timing_driven_flag_(timing_driven_flag)
  {
    coarsener_ = coarsener;
    partitioner_ = partitioner;
    k_way_refiner_ = k_way_pm_refiner;
    k_way_pm_refiner_ = k_way_pm_refiner;
    greedy_refiner_ = greedy_refiner;
    ilp_refiner_ = ilp_refiner;
    evaluator_ = evaluator;
    logger_ = logger;
  }

  // Main function
  // here the hgraph should not be const
  // Because our slack-rebudgeting algorithm will change hgraph
  TP_partition Partition(HGraph hgraph,
                         const matrix<float>& max_block_balance);

 private:
  void InitialPartition(const HGraph hgraph, 
                          matrix<float>& max_block_balance,
                          matrix<int>& top_initial_solutions,
                          int& best_solution_id) const;
    
  void RefinePartition(TP_coarse_graphs hierarchy,
                         const matrix<float>& max_block_balance,
                         matrix<int>& top_solutions,
                         int& best_solution_id) const;
    
  void VcycleRefinement(HGraph hgraph, 
                          const matrix<float>& max_block_balance,
                          std::vector<int>& best_solution,
                          float& best_cost) const;
    
  void CallRefiner(const HGraph hgraph, 
                     const matrix<float>& max_block_balance,
                     std::vector<int>& solution) const;

  void SingleLevelPartition(HGraph hgraph,
                            const matrix<float>& max_block_balance) const;

  // basic parameters
  const int num_parts_ = 2; 

  // user-specified parameters
  const int num_initial_random_solutions_ = 50;
  const int num_best_initial_solutions_ = 10;
  const int max_num_vcycle_ = 10; // maximum number of vcycles
  const int seed_ = 0; // random seed
  const bool timing_driven_flag_ = true;

  // pointers
  TP_coarsening_ptr coarsener_ = nullptr;
  TP_partitioning_ptr partitioner_ = nullptr;
  TP_k_way_refiner_ptr k_way_refiner_ = nullptr;
  TP_k_way_pm_refiner_ptr k_way_pm_refiner_ = nullptr; 
  TP_greedy_refiner_ptr greedy_refiner_ = nullptr;
  TP_ilp_refiner_ptr ilp_refiner_ = nullptr;
  TP_evaluator_ptr evaluator_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace par

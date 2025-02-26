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

#include <memory>
#include <vector>

#include "Coarsener.h"
#include "Evaluator.h"
#include "GreedyRefine.h"
#include "Hypergraph.h"
#include "ILPRefine.h"
#include "KWayFMRefine.h"
#include "KWayPMRefine.h"
#include "Partitioner.h"
#include "Utilities.h"
#include "utl/Logger.h"

namespace par {

// Multilevel partitioner
class MultilevelPartitioner;
using MultiLevelPartitioner = std::shared_ptr<MultilevelPartitioner>;

class MultilevelPartitioner
{
 public:
  MultilevelPartitioner(int num_parts,
                        // user-specified parameters
                        bool v_cycle_flag,
                        int num_initial_solutions,
                        int num_best_initial_solutions,
                        int num_vertices_threshold_ilp,
                        int max_num_vcycle,
                        int num_coarsen_solutions,
                        int seed,
                        // pointers
                        CoarseningPtr coarsener,
                        PartitioningPtr partitioner,
                        KWayFMRefinerPtr k_way_fm_refiner,
                        KWayPMRefinerPtr k_way_pm_refiner,
                        GreedyRefinerPtr greedy_refiner,
                        IlpRefinerPtr ilp_refiner,
                        EvaluatorPtr evaluator,
                        utl::Logger* logger);

  // Main function
  // here the hgraph should not be const
  // Because our slack-rebudgeting algorithm will change hgraph
  Partitions Partition(const HGraphPtr& hgraph,
                       const Matrix<float>& upper_block_balance,
                       const Matrix<float>& lower_block_balance) const;

  // Use the initial solution as the community feature
  // Call Vcycle refinement
  void VcycleRefinement(const HGraphPtr& hgraph,
                        const Matrix<float>& upper_block_balance,
                        const Matrix<float>& lower_block_balance,
                        std::vector<int>& best_solution) const;

 private:
  // Run single-level partitioning
  std::vector<int> SingleLevelPartition(
      const HGraphPtr& hgraph,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance) const;

  // We expose this interface for the last-minute improvement
  std::vector<int> SingleCycleRefinement(
      const HGraphPtr& hgraph,
      const Matrix<float>& upper_block_balance,
      const Matrix<float>& lower_block_balance) const;

  // Generate initial partitioning
  // Include random partitioning, Vile partitioning and ILP partitioning
  void InitialPartition(const HGraphPtr& hgraph,
                        const Matrix<float>& upper_block_balance,
                        const Matrix<float>& lower_block_balance,
                        Matrix<int>& top_initial_solutions,
                        int& best_solution_id) const;

  // Refine the solutions in top_solutions in parallel with multi-threading
  // the top_solutions and best_solution_id will be updated during this process
  void RefinePartition(CoarseGraphPtrs hierarchy,
                       const Matrix<float>& upper_block_balance,
                       const Matrix<float>& lower_block_balance,
                       Matrix<int>& top_solutions,
                       int& best_solution_id) const;

  // For last minute improvement
  // Refine function
  // Ilp refinement, k_way_pm_refinement,
  // k_way_fm_refinement and greedy refinement
  void CallRefiner(const HGraphPtr& hgraph,
                   const Matrix<float>& upper_block_balance,
                   const Matrix<float>& lower_block_balance,
                   std::vector<int>& solution) const;

  // Perform cut-overlay clustering and ILP-based partitioning
  // The ILP-based partitioning uses top_solutions[best_solution_id] as a hint,
  // such that the runtime can be signficantly reduced
  std::vector<int> CutOverlayILPPart(const HGraphPtr& hgraph,
                                     const Matrix<float>& upper_block_balance,
                                     const Matrix<float>& lower_block_balance,
                                     const Matrix<int>& top_solutions,
                                     int best_solution_id) const;

  // basic parameters
  const int num_parts_ = 2;
  const int num_vertices_threshold_ilp_
      = 20;  // number of vertices used for ILP partitioning

  // user-specified parameters
  const int num_initial_random_solutions_ = 50;
  const int num_best_initial_solutions_ = 10;
  const int max_num_vcycle_ = 10;  // maximum number of vcycles
  const int num_coarsen_solutions_
      = 3;  // number of coarsening solutions with different random seed
  const int seed_ = 0;  // random seed
  const bool v_cycle_flag_ = true;

  // pointers
  CoarseningPtr coarsener_ = nullptr;
  PartitioningPtr partitioner_ = nullptr;
  KWayFMRefinerPtr k_way_fm_refiner_ = nullptr;
  KWayPMRefinerPtr k_way_pm_refiner_ = nullptr;
  GreedyRefinerPtr greedy_refiner_ = nullptr;
  IlpRefinerPtr ilp_refiner_ = nullptr;
  EvaluatorPtr evaluator_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace par

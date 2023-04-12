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

#include "TPMultilevel.h"
#include "TPEvaluator.h"
#include "TPHypergraph.h"
#include "TPPartitioner.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {


// Main function
// here the hgraph should not be const
// Because our slack-rebudgeting algorithm will change hgraph
TP_partition TPmultilevelPartitioner::Partition(HGraph hgraph,
                                                const matrix<float>& max_block_balance) const
{
  // Main implementation
  // Step 1: run coarsening
  // Step 2: run initial partitioning
  // Step 3: run refinement
  // Step 4: Guided v-cycle

  // Step 1: - Step 3
  TP_partition best_solution = SingleLevelPartition(hgraph, max_block_balance);

  // Step 4: Guided v-cycle. Note that hgraph has been updated.
  // The best_solution will be refined.
  // The initial value of best solution will be used to guide the coarsening process
  // and use as the initial solution
  if (v_cycle_flag_ == true) {
    VcycleRefinement(hgraph, max_block_balance, best_solution);   
  }

  return best_solution;
}


// Private functions (Utilities)

// Run single-level partitioning
TP_partition TPmultilevelPartitioner::SingleLevelPartition(HGraph hgraph,
                                  const matrix<float>& max_block_balance) const
{
  // Step 1: run coarsening
  // Step 2: run initial partitioning
  // Step 3: run refinement

  // Step 1: run coarsening
  TP_coarse_graphs hierarchy = coarsener_->LazyFirstChoice(hgraph);
  
  // Step 2: run initial partitioning
  HGraph coarsest_hgraph = hierarchy.back();
  
  // pick top num_best_initial_solutions_ solutions from
  // num_initial_random_solutions_ solutions
  // here we reserve num_vertices_ for each top_solution
  matrix<int> top_solutions;
  for (int i = 0; i < num_best_initial_solutions_; i++) {
    std::vector<int> solution { };
    top_solutions.push_back(solution);
    top_solutions.back().reserve(hgraph->num_vertices_);  // reserve the size
  }
  int best_solution_id = -1; 
  InitialPartition(coarsest_hgraph, max_block_balance, top_solutions, best_solution_id);

  // Step 3: run refinement 
  // Here we need to do rebugetting on the best solution
  RefinePartition(hierarchy, 
                  max_block_balance,
                  top_solutions,
                  best_solution_id);  

  return top_solutions[best_solution_id];
}


// Use the initial solution as the community feature
// Call Vcycle refinement
void TPmultilevelPartitioner::VcycleRefinement(HGraph hgraph, 
                      const matrix<float>& max_block_balance,
                             std::vector<int>& best_solution,
                             float& best_cost) const
{
  for (int num_cycles = 0; num_cycles < max_num_vcycle_; num_cycles++) {
    // use the initial solution as the community feature
    hgraph->community_flag_ = true;
    hgraph->community_attr_ = best_solution;
    best_solution = SingleLevelPartition(hgraph, max_block_balance);
    const float cost = evaluator_->CutEvaluator(hgraph, best_solution).first;
    if (cost == best_cost) {
      return;
    } else {
      best_cost = cost; // the best_cost can only decrease
      logger_->report("[INFO][V-cycle Refinement] num_cycles = {}, cutcost = {}", num_cycles, best_cost);
    }            
  } 
}

// Generate initial partitioning
// Include random partitioning, Vile partitioning and ILP partitioning
void TPmultilevelPartitioner::InitialPartition(const HGraph hgraph, 
                                  matrix<float>& max_block_balance,
                                matrix<int>& top_initial_solutions,
                                             int& best_solution_id) const 
{
  logger_->report("======================================================================");
  logger_->report("[STATUS] Initial Partitioning ");
  logger_->report("======================================================================");
  std::mt19937 gen;
  gen.seed(seed_);
  std::uniform_real_distribution<> dist(0.0, 1.0);
  std::vector<float> initial_solutions_cost;
  matrix<int> initial_solutions;
  if (timing_driven_flag_ == false && num_parts_ <= 2) {
    // random partitioning + Vile + ILP
    initial_solutions.resize(num_initial_random_solutions_ + 2);
  } else {
    // random partitioning + Vile
    initial_solutions.resize(num_initial_random_solutions_ + 1);
  }
  // generate random seed
  for (int i = 0; i < num_initial_random_solutions_ ; ++i) {
    const int seed = std::numeric_limits<int>::max() * dist(gen);
    auto& solution = initial_solutions[i];
    // call random partitioning
    partitioner_->SetRandomSeed(seed);
    partitioner_->Partition(hgraph, max_block_balance, solution, PartitionType::INIT_RANDOM);
    // call refiner to improve the solution
    CallRefiner(hgraph, max_block_balance, solution);
    const float cut_cost = evaluator_->CutEvaluator(hgraph, solution).first;
    initial_solutions_cost.push_back(cut_cost);
    logger_->report("[INIT-PART] {} :: Random part cutcost {}", i, top_initial_solutions_cost.back());
  }
  // Vile partitioning. Vile partitioning needs refiner to generated a balanced partitioning
  auto& vile_solution = initial_solutions[num_initial_random_solutions_];
  partitioner_->Partition(hgraph, max_block_balance, vile_solution, PartitionType::INIT_VILE);
  // We need k_way_fm_refiner to generate a balanced partitioning
  k_way_fm_refiner_->SetMaxMove(hgraph->num_vertices_);
  k_way_fm_refiner_->Refine(hgraph, max_block_balance, vile_solution);
  k_way_fm_refiner_->RestoreDefaultParameters();
  CallRefiner(hgraph, max_block_balance, vile_solution);
  initial_solutions_cost.push_back(evaluator_->CutEvaluator(hgraph, vile_solution).first);
  logger_->report("[INIT-PART] :: VILE part cutcost {}", top_initial_solutions_cost.back());
  // ILP partitioning
  if (timing_driven_flag_ == false && num_parts_ <= 2) {
    auto& ilp_solution = initial_solutions.back();
    partitioner_->Partition(hgraph, max_block_balance, ilp_solution, PartitionType::INIT_DIRECT_ILP);
    CallRefiner(hgraph, max_block_balance, ilp_solution);
    initial_solutions_cost.push_back(evaluator_->CutEvaluator(hgraph, ilp_solution).first);
    logger_->report("[INIT-PART] :: ILP part cutcost {}", top_initial_solutions_cost.back());
  }
  // sort the solutions based on cost
  std::vector<int> solution_ids(top_initial_solutions_cost.size(), 0);
  std::stoi(solution_ids.begin(), solution_ids.end(), 0);
  // define compare function
  auto lambda_sort_criteria = [&](int& x, int& y) -> bool {
    return initial_solutions_cost[x] < initial_solutions_cost[y];
  };
  std::sort(solution_ids.begin(), solution_ids.end(), lambda_sort_criteria);
  // pick the top num_best_initial_solutions_ solutions
  for (int i = 0; i < num_best_initial_solutions_; i++) {
    top_initial_solutions[i] = initial_solutions[solution_ids[i]];
  }
  best_solution_id = 0; // the first one is the best one
  logger_->report("[INIT-PART] :: Best initial cutcost {}", top_initial_solutions_cost[solution_ids[best_solution_id]]);
}


// Refine the solutions in top_solutions in parallel with multi-threading
// the top_solutions and best_solution_id will be updated during this process
void TPmultilevelPartitioner::RefinePartition(TP_coarse_graphs hierarchy,
                                  const matrix<float>& max_block_balance,
                                              matrix<int>& top_solutions,
                                                   int& best_solution_id) const
{
  int num_level = 0;
  // rebudget based on the best solution
  auto hgraph_iter = hierarchy.rbegin();
  while (hgraph_iter != hierarchy.rend()) {
    HGraph coarse_hgraph = *hgraph_iter;
    hgraph_iter++;
    HGraph hgraph = *hgraph_iter;
    // convert the solution in coarse_hgraph to the solution of hgraph
    for (auto i = 0; i < top_solutions.size(); i++) {
      std::vector<int> refined_solution;
      refined_solution.resize(hgraph->num_vertices_);
      for (int cluster_id = 0; cluster_id < coarse_hgraph->num_vertices_; cluster_id++) {
        const int part_id = top_solutions[i][cluster_id];
        for (const auto& v : coarse_hgraph->vertex_c_attr_[cluster_id]) {
          refined_solution_[v] = part_id;
        }
      }
      top_solutions[i] = refined_solution;  
    }
    // do slack rebudgetting based on best solution
    if (timing_driven_flag_ == true) {
      evaluator_->UpdateTiming(hgraph, top_solutions[best_solution_id]);
    }
    // Parallel refine all the solutions
    std::vector<std::thread> threads;
    for (auto i = 0; i < num_best_initial_solutions_; i++) {
      threads.push_back(std::thread(&par::TPmultilevelPartitioner::CallRefiner,
                                    this, 
                                    hgraph,
                                    std::ref(max_block_balance),
                                    std::ref(top_solutions[i])));
    }    
    for (auto& th : threads) {
      th.join();
    }
    threads.clear();
    // update the best_solution_id
    int best_cost = std::numeric_limits<float>::max();
    for (auto i = 0; i < num_best_initial_solutions_; i++) {
      const float cost = evaluator_->CutEvaluator(finer_hypergraph, top_solutions[i]).first;
      if (best_cost < cost) {
        best_cost = cost;
        best_solution_id = i;
      }
    }
    logger_->report("[Refinement] Level {} :: num_vertices = {}, num_hyperedges = {},"
                    " cutcost = {}, best_solution_id = {}", 
                     ++num_level,
                     finer_hypergraph->num_vertices_,
                     finer_hypergraph->num_hyperedges_,
                     best_cost, best_solution_id);
  }
}

// Refine function
// Ilp refinement, k_way_pm_refinement, 
// k_way_fm_refinement and greedy refinement
void TPmultilevelPartitioner::CallRefiner(const HGraph hgraph, 
                       const matrix<float>& max_block_balance,
                                   std::vector<int>& solution) const
{
  if (timing_driven_flag_ == false && num_parts_ == 2) {
    ilp_refiner_->Refine(hgraph, max_block_balance, solution);
  }
  k_way_pm_refiner_->Refine(hgraph, max_block_balance, solution);
  k_way_fm_refiner_->Refine(hgraph, max_block_balance, solution);  
  greedy_refiner_->Refine(hgraph, max_block_balance, solution);
}

}  // namespace par

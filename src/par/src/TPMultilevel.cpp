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
#include <functional>
#include "TPEvaluator.h"
#include "TPHypergraph.h"
#include "TPPartitioner.h"
#include "utl/Logger.h"

using utl::PAR;

namespace par {

// Main function
// here the hgraph should not be const
// Because our slack-rebudgeting algorithm will change hgraph
TP_partition TPmultilevelPartitioner::Partition(HGraphPtr hgraph,
                                                const MATRIX<float>& upper_block_balance,
                                                const MATRIX<float>& lower_block_balance) const
{
  // Main implementation
  // Step 1: run coarsening
  // Step 2: run initial partitioning
  // Step 3: run refinement
  // Step 4: Guided v-cycle

  // Step 1: - Step 3
  std::vector<int> best_solution = SingleLevelPartition(hgraph, upper_block_balance, lower_block_balance);

  // Step 4: Guided v-cycle. Note that hgraph has been updated.
  // The best_solution will be refined.
  // The initial value of best solution will be used to guide the coarsening process
  // and use as the initial solution
  if (v_cycle_flag_ == true) {
    VcycleRefinement(hgraph, upper_block_balance, lower_block_balance, best_solution);   
  }

  return best_solution;
}


// Private functions (Utilities)

// Run single-level partitioning
TP_partition TPmultilevelPartitioner::SingleLevelPartition(HGraphPtr hgraph,
                                  const MATRIX<float>& upper_block_balance,
                                  const MATRIX<float>& lower_block_balance) const
{
  // Step 1: run coarsening
  // Step 2: run initial partitioning
  // Step 3: run refinement
  // Step 4: cut-overlay clustering and ILP-based partitioning

  // Step 1: run coarsening
  TP_coarse_graph_ptrs hierarchy = coarsener_->LazyFirstChoice(hgraph);
  
  // Step 2: run initial partitioning
  HGraphPtr coarsest_hgraph = hierarchy.back();

  // pick top num_best_initial_solutions_ solutions from
  // num_initial_random_solutions_ solutions
  // here we reserve num_vertices_ for each top_solution
  MATRIX<int> top_solutions;
  for (int i = 0; i < num_best_initial_solutions_; i++) {
    std::vector<int> solution { };
    top_solutions.push_back(solution);
    top_solutions.back().reserve(hgraph->num_vertices_);  // reserve the size
  }
  int best_solution_id = -1;
  InitialPartition(coarsest_hgraph, upper_block_balance, lower_block_balance, top_solutions, best_solution_id);

  // Step 3: run refinement 
  // Here we need to do rebugetting on the best solution
  RefinePartition(hierarchy, 
                  upper_block_balance,
                  lower_block_balance,
                  top_solutions,
                  best_solution_id);  

  // Step 4: cut-overlay clustering and ILP-based partitioning
  // Perform cut-overlay clustering and ILP-based partitioning
  // The ILP-based partitioning uses top_solutions[best_solution_id] as a hint,
  // such that the runtime can be signficantly reduced
  return CutOverlayILPPart(hgraph, upper_block_balance, lower_block_balance, top_solutions, best_solution_id);
  //return top_solutions[best_solution_id];
}


// Use the initial solution as the community feature
// Call Vcycle refinement
void TPmultilevelPartitioner::VcycleRefinement(HGraphPtr hgraph, 
                       const MATRIX<float>& upper_block_balance,
                       const MATRIX<float>& lower_block_balance,
                       std::vector<int>& best_solution) const
{
  float best_cost = evaluator_->CutEvaluator(hgraph, best_solution).first;
  MATRIX<int> candidate_solutions;
  for (int num_cycles = 0; num_cycles < max_num_vcycle_; num_cycles++) {
    // use the initial solution as the community feature
    hgraph->community_flag_ = true;
    hgraph->community_attr_ = best_solution;
    best_solution = SingleCycleRefinement(hgraph, upper_block_balance, lower_block_balance);
    candidate_solutions.push_back(best_solution);
    const float cost = evaluator_->CutEvaluator(hgraph, best_solution).first;
    if (cost == best_cost) {
      break;
    } else {
      best_cost = cost; // the best_cost can only decrease
      logger_->report("[INFO][V-cycle Refinement] num_cycles = {}, cutcost = {}", num_cycles, best_cost);
    }            
  }
  // Perform Cut-overlay clustering and ILP-based partitioning
  best_solution = CutOverlayILPPart(hgraph, 
                                    upper_block_balance, 
                                    lower_block_balance,
                                    candidate_solutions, 
                                    static_cast<int>(candidate_solutions.size()) - 1);
}

// Single Vcycle Refinement
std::vector<int> TPmultilevelPartitioner::SingleCycleRefinement(HGraphPtr hgraph,
                                        const MATRIX<float>& upper_block_balance,
                                        const MATRIX<float>& lower_block_balance) const
{
  // Step 1: run coarsening
  // Step 2: run refinement

  // Step 1: run coarsening
  TP_coarse_graph_ptrs hierarchy = coarsener_->LazyFirstChoice(hgraph);
  
  // Step 2: run initial refinement
  HGraphPtr coarsest_hgraph = hierarchy.back();
  MATRIX<int> top_solutions { coarsest_hgraph->community_attr_ };
  int best_solution_id = 0; // only one solution
  if (coarsest_hgraph->num_vertices_ <= num_clusters_threshold_overlay_) {
    partitioner_->Partition(coarsest_hgraph, 
                            upper_block_balance,
                            lower_block_balance,
                            top_solutions[best_solution_id], 
                            PartitionType::INIT_DIRECT_ILP);
  }
  // Here we need to do rebugetting on the best solution
  RefinePartition(hierarchy, 
                  upper_block_balance,
                  lower_block_balance,
                  top_solutions,
                  best_solution_id);  
  return top_solutions[best_solution_id];
}

// Generate initial partitioning
// Include random partitioning, Vile partitioning and ILP partitioning
void TPmultilevelPartitioner::InitialPartition(const HGraphPtr hgraph, 
                                const MATRIX<float>& upper_block_balance,
                                const MATRIX<float>& lower_block_balance,
                                MATRIX<int>& top_initial_solutions,
                                             int& best_solution_id) const 
{
  logger_->report("======================================================================");
  logger_->report("[STATUS] Initial Partitioning ");
  logger_->report("======================================================================");
  std::mt19937 gen;
  gen.seed(seed_);
  std::uniform_real_distribution<> dist(0.0, 1.0);
  std::vector<float> initial_solutions_cost;
  std::vector<bool> initial_solutions_flag; // if the solutions statisfy balance constraint
  MATRIX<int> initial_solutions;
  if (timing_driven_flag_ == false) {
    // random partitioning + Vile + ILP
    initial_solutions.resize(num_initial_random_solutions_ + 2);
  } else {
    // random partitioning + Vile
    initial_solutions.resize(num_initial_random_solutions_ + 1);
  }
  // We need k_way_fm_refiner to generate a balanced partitioning
  k_way_fm_refiner_->SetMaxMove(hgraph->num_vertices_);
  // generate random seed
  for (int i = 0; i < num_initial_random_solutions_ ; ++i) {
    const int seed = std::numeric_limits<int>::max() * dist(gen);
    auto& solution = initial_solutions[i];
    // call random partitioning
    partitioner_->SetRandomSeed(seed);
    partitioner_->Partition(hgraph, upper_block_balance, lower_block_balance, solution, PartitionType::INIT_RANDOM);
    // call FM refiner to improve the solution
    k_way_fm_refiner_->Refine(hgraph, upper_block_balance, lower_block_balance, solution);
    const std::pair<float, MATRIX<float> > token = evaluator_->CutEvaluator(hgraph, solution, true);
    initial_solutions_cost.push_back(token.first);
    //initial_solutions_flag.push_back((token.second <= upper_block_balance) && (lower_block_balance <= token.second));
    initial_solutions_flag.push_back(token.second <= upper_block_balance);
    logger_->report("[INIT-PART] {} :: Random part cutcost {}, balance_flag = {}", 
                     i, initial_solutions_cost.back(), initial_solutions_flag.back());
  }
  // Vile partitioning. Vile partitioning needs refiner to generated a balanced partitioning
  auto& vile_solution = initial_solutions[num_initial_random_solutions_];
  partitioner_->Partition(hgraph, upper_block_balance, lower_block_balance, vile_solution, PartitionType::INIT_VILE);
  // We need k_way_fm_refiner to generate a balanced partitioning
  k_way_fm_refiner_->Refine(hgraph, upper_block_balance, lower_block_balance, vile_solution);
  k_way_fm_refiner_->RestoreDefaultParameters();
  CallRefiner(hgraph, upper_block_balance, lower_block_balance, vile_solution);
  const std::pair<float, MATRIX<float> > vile_token = evaluator_->CutEvaluator(hgraph, vile_solution);
  initial_solutions_cost.push_back(vile_token.first);
  //initial_solutions_flag.push_back((vile_token.second <= upper_block_balance) && (lower_block_balance <= vile_token.second));
  initial_solutions_flag.push_back(vile_token.second <= upper_block_balance);
  logger_->report("[INIT-PART] :: VILE part cutcost {}", initial_solutions_cost.back());
  // ILP partitioning
  if (timing_driven_flag_ == false) {
    auto& ilp_solution = initial_solutions.back();
    const int seed = std::numeric_limits<int>::max() * dist(gen);
    // call random partitioning
    partitioner_->SetRandomSeed(seed);
    partitioner_->Partition(hgraph, upper_block_balance, lower_block_balance, ilp_solution, PartitionType::INIT_RANDOM);
    partitioner_->Partition(hgraph, upper_block_balance, lower_block_balance, ilp_solution, PartitionType::INIT_DIRECT_ILP);
    const std::pair<float, MATRIX<float> > ilp_token = evaluator_->CutEvaluator(hgraph, vile_solution);
    initial_solutions_cost.push_back(ilp_token.first);
    initial_solutions_flag.push_back(ilp_token.second <= upper_block_balance);
    //initial_solutions_flag.push_back((ilp_token.second <= upper_block_balance) && (lower_block_balance <= ilp_token.second));
    logger_->report("[INIT-PART] :: ILP part cutcost {}", initial_solutions_cost.back());
  }
  // sort the solutions based on cost
  std::vector<int> solution_ids(initial_solutions_cost.size(), 0);
  std::iota(solution_ids.begin(), solution_ids.end(), 0);
  // define compare function
  auto lambda_sort_criteria = [&](int& x, int& y) -> bool {
    return initial_solutions_cost[x] < initial_solutions_cost[y];
  };
  std::sort(solution_ids.begin(), solution_ids.end(), lambda_sort_criteria);
  // pick the top num_best_initial_solutions_ solutions
  // while satisfying the balance constraint
  // TODO: maybe some relaxtation can be tuned here
  int num_chosen_best_init_solution = 0;
  float best_initial_cost = 0.0;
  for (auto id : solution_ids) {
    if (initial_solutions_flag[id] == true) {
      top_initial_solutions[num_chosen_best_init_solution] = initial_solutions[id];
      num_chosen_best_init_solution++;
      if (num_chosen_best_init_solution == 1) {
        best_initial_cost = initial_solutions_cost[id];
      }
      if (num_chosen_best_init_solution >= num_best_initial_solutions_) {
        break;
      }
    }
  } 
  // Check if there are some valid solutions
  if (num_chosen_best_init_solution == 0) {
    num_chosen_best_init_solution++;
    top_initial_solutions[0] = initial_solutions[solution_ids[0]];
  } 

  // remove invalid solution
  while (top_initial_solutions.size() > num_chosen_best_init_solution) {
    top_initial_solutions.pop_back();
  }
  logger_->report("[INFO] Number of chosen best initial solutions = {}", 
                  num_chosen_best_init_solution);
  best_solution_id = 0; // the first one is the best one
  logger_->report("[INIT-PART] :: Best initial cutcost {}", best_initial_cost);
}


// Refine the solutions in top_solutions in parallel with multi-threading
// the top_solutions and best_solution_id will be updated during this process
void TPmultilevelPartitioner::RefinePartition(TP_coarse_graph_ptrs hierarchy,
                                    const MATRIX<float>& upper_block_balance,
                                    const MATRIX<float>& lower_block_balance,
                                                  MATRIX<int>& top_solutions,
                                                 int& best_solution_id) const
{
  if (hierarchy.size() <= 1) {
    return; // no need to refine.  
  }
  
  int num_level = 0;
  // rebudget based on the best solution
  auto hgraph_iter = hierarchy.rbegin();
  while (hgraph_iter != hierarchy.rend()) {
    HGraphPtr coarse_hgraph = *hgraph_iter;
    hgraph_iter++;
    if (hgraph_iter == hierarchy.rend()) {
      return;
    }
    HGraphPtr hgraph = *hgraph_iter;
    // convert the solution in coarse_hgraph to the solution of hgraph
    for (auto i = 0; i < top_solutions.size(); i++) {
      std::vector<int> refined_solution;
      refined_solution.resize(hgraph->num_vertices_);
      for (int cluster_id = 0; cluster_id < coarse_hgraph->num_vertices_; cluster_id++) {
        const int part_id = top_solutions[i][cluster_id];
        for (const auto& v : coarse_hgraph->vertex_c_attr_[cluster_id]) {
          refined_solution[v] = part_id;
        }
      }
      top_solutions[i] = refined_solution;
    }
    // Parallel refine all the solutions
    std::vector<std::thread> threads;
    for (auto i = 0; i < top_solutions.size(); i++) {
      threads.push_back(std::thread(&par::TPmultilevelPartitioner::CallRefiner,
                                    this, 
                                    hgraph,
                                    std::ref(upper_block_balance),
                                    std::ref(lower_block_balance),
                                    std::ref(top_solutions[i])));
    }    
    for (auto& th : threads) {
      th.join();
    }
    threads.clear();
    // update the best_solution_id
    float best_cost = std::numeric_limits<float>::max();
    for (auto i = 0; i < top_solutions.size(); i++) {
      const float cost = evaluator_->CutEvaluator(hgraph, top_solutions[i], true).first;
      if (best_cost > cost) {
        best_cost = cost;
        best_solution_id = i;
      }
    }
    logger_->report("[Refinement] Level {} :: num_vertices = {}, num_hyperedges = {},"
                    " cutcost = {}, best_solution_id = {}", 
                     ++num_level,
                     hgraph->num_vertices_,
                     hgraph->num_hyperedges_,
                     best_cost, best_solution_id);
  }
}

// Refine function
// k_way_pm_refinement, 
// k_way_fm_refinement and greedy refinement
void TPmultilevelPartitioner::CallRefiner(const HGraphPtr hgraph, 
                        const MATRIX<float>& upper_block_balance,
                        const MATRIX<float>& lower_block_balance,
                                   std::vector<int>& solution) const
{
  //if (num_parts_ > 2) { // Pair-wise FM only used for multi-way partitioning
  //  k_way_pm_refiner_->Refine(hgraph, upper_block_balance, lower_block_balance, solution);
  //} 
  k_way_fm_refiner_->Refine(hgraph, upper_block_balance, lower_block_balance, solution);
  //greedy_refiner_->Refine(hgraph, upper_block_balance, lower_block_balance, solution);
}

// Perform cut-overlay clustering and ILP-based partitioning
// The ILP-based partitioning uses top_solutions[best_solution_id] as a hint,
// such that the runtime can be signficantly reduced
std::vector<int> TPmultilevelPartitioner::CutOverlayILPPart(const HGraphPtr hgraph,
                                          const MATRIX<float>& upper_block_balance,
                                          const MATRIX<float>& lower_block_balance,
                                          const MATRIX<int>& top_solutions,
                                          int best_solution_id) const
{
  std::vector<int> optimal_solution = top_solutions[best_solution_id];
  std::vector<int> vertex_cluster_vec(hgraph->num_vertices_, -1);
  // check if the hyperedge is cut by solutions
  std::vector<bool> hyperedge_mask(hgraph->num_hyperedges_, false); 
  for (const auto& solution : top_solutions) {
    for (int e = 0; e < hgraph->num_hyperedges_; e++) {
      if (hyperedge_mask[e] == true) {
        continue; // This hyperedge has been cut
      }
      const int start_idx = hgraph->eptr_[e];
      const int end_idx = hgraph->eptr_[e + 1];
      const int block_id = solution[hgraph->eind_[start_idx]];
      for (int idx = start_idx + 1; idx < end_idx; idx++) {
        if (solution[hgraph->eind_[idx]] != block_id) {
          hyperedge_mask[e] = true;
          break; // end this hyperedge
        }
      }
    }
  }
  // pre-order DFS to traverse the hypergraph
  // Declaration
  auto lambda_detect_connected_components = [&](int v, int cluster_id, auto&& lambda_detect_connected_components) -> void {
    const int start_idx = hgraph->vptr_[v];
    const int end_idx = hgraph->vptr_[v] + 1;
    for (int idx = start_idx; idx < end_idx; idx++) {
      const int e = hgraph->vind_[idx];
      if (hyperedge_mask[e] == true) {
        continue; // this hyperedge has been cut
      }
      for (auto v_idx = hgraph->eptr_[e]; v_idx < hgraph->eptr_[e + 1]; v_idx++) {
        const int u = hgraph->eind_[v_idx]; // vertex u
        if (vertex_cluster_vec[u] == -1) {
          vertex_cluster_vec[u] = cluster_id;
          lambda_detect_connected_components(u, cluster_id, lambda_detect_connected_components);          
        }    
      }
    }    
  };

  // detect the connected components and mask each connected component as a cluster
  int cluster_id = -1;  
  // map the initial optimal solution to the solution of clustered hgraph
  std::vector<int> init_solution;
  for (int v = 0; v < hgraph->num_vertices_; v++) {
    if (vertex_cluster_vec[v] == -1) {
      vertex_cluster_vec[v] = ++cluster_id;
      init_solution.push_back(top_solutions[best_solution_id][v]);
      lambda_detect_connected_components(v, cluster_id, lambda_detect_connected_components);
    }
  }

  const int num_clusters = cluster_id + 1;
  std::vector<std::vector<int> > cluster_attr;
  cluster_attr.reserve(num_clusters);
  for (int id = 0; id < num_clusters; id++) {
    std::vector<int> group_cluster { };
    cluster_attr.push_back(group_cluster);
  }
  for (int v = 0; v < hgraph->num_vertices_; v++) {
    cluster_attr[vertex_cluster_vec[v]].push_back(v);
  }

  // Call ILP-based partitioning
  HGraphPtr clustered_hgraph = coarsener_->GroupVertices(hgraph, cluster_attr);
  logger_->report("[INFO] Cut-Overlay Clustering : num_vertices = {}, num_hyperedges = {}",
                   clustered_hgraph->num_vertices_, clustered_hgraph->num_hyperedges_);

  if (num_clusters <= num_clusters_threshold_overlay_) {
    partitioner_->Partition(clustered_hgraph, 
                            upper_block_balance, 
                            lower_block_balance,  
                            init_solution, 
                            PartitionType::INIT_DIRECT_ILP);
  } else {
    clustered_hgraph->community_flag_ = true;
    clustered_hgraph->community_attr_ = init_solution;
    init_solution = SingleCycleRefinement(clustered_hgraph, upper_block_balance, lower_block_balance);
  }

  // map the solution back to the original hypergraph
  for (int c_id = 0; c_id < clustered_hgraph->num_vertices_; c_id++) {
    const int block_id = init_solution[c_id];
    for (const auto& v : clustered_hgraph->vertex_c_attr_[c_id]) {
      optimal_solution[v] = block_id;
    }
  }
  
  logger_->report("[INFO] Statistics of cut-overlay solution:"); 
  evaluator_->CutEvaluator(hgraph, optimal_solution, true);
  return optimal_solution;
}

}  // namespace par

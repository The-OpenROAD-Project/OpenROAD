///////////////////////////////////////////////////////////////////////////
//
// BSD 3-Clause License
//
// Copyright (c) 2019, The Regents of the University of California
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

#include <map>
#include <set>
#include <string>
#include <vector>

namespace ord {
class dbVerilogNetwork;
}

namespace odb {
class dbDatabase;
class dbChip;
class dbBlock;
}  // namespace odb

namespace sta {
class dbNetwork;
class Instance;
class NetworkReader;
class Library;
class Port;
class Net;
class dbSta;
}  // namespace sta

namespace utl {
class Logger;
}

namespace par {

struct CompareInstancePtr
{
 public:
  CompareInstancePtr(sta::dbNetwork* db_network = nullptr)
      : db_network_(db_network)
  {
  }
  bool operator()(const sta::Instance* lhs, const sta::Instance* rhs) const;

 private:
  sta::dbNetwork* db_network_ = nullptr;
};

class PartitionMgr
{
 public:
  void init(odb::dbDatabase* db,
            sta::dbNetwork* db_network,
            sta::dbSta* sta,
            utl::Logger* logger);

  // The function for partitioning a hypergraph
  // This is used for replacing hMETIS
  // Key supports:
  // (1) fixed vertices constraint in fixed_file
  // (2) community attributes in community_file (This can be used to guide the
  // partitioning process) (3) stay together attributes in group_file. (4)
  // placement information is specified in placement file The format is that
  // each line cooresponds to a group fixed vertices, community and placement
  // attributes both follows the hMETIS format
  void tritonPartHypergraph(unsigned int num_parts,
                            float balance_constraint,
                            const std::vector<float>& base_balance,
                            const std::vector<float>& scale_factor,
                            unsigned int seed,
                            int vertex_dimension,
                            int hyperedge_dimension,
                            int placement_dimension,
                            const char* hypergraph_file,
                            const char* fixed_file,
                            const char* community_file,
                            const char* group_file,
                            const char* placement_file,
                            // weight parameters
                            const std::vector<float>& e_wt_factors,
                            const std::vector<float>& v_wt_factors,
                            const std::vector<float>& placement_wt_factors,
                            // coarsening related parameters
                            int thr_coarsen_hyperedge_size_skip,
                            int thr_coarsen_vertices,
                            int thr_coarsen_hyperedges,
                            float coarsening_ratio,
                            int max_coarsen_iters,
                            float adj_diff_ratio,
                            int min_num_vertices_each_part,
                            // initial partitioning related parameters
                            int num_initial_solutions,
                            int num_best_initial_solutions,
                            // refinement related parameters
                            int refiner_iters,
                            int max_moves,
                            float early_stop_ratio,
                            int total_corking_passes,
                            // vcycle related parameters
                            bool v_cycle_flag,
                            int max_num_vcycle,
                            int num_coarsen_solutions,
                            int num_vertices_threshold_ilp,
                            int global_net_threshold);

  // Evaluate a given solution of a hypergraph
  // The fixed vertices should statisfy the fixed vertices constraint
  // The group of vertices should stay together in the solution
  // The vertex balance should be satisfied
  void evaluateHypergraphSolution(unsigned int num_parts,
                                  float balance_constraint,
                                  const std::vector<float>& base_balance,
                                  const std::vector<float>& scale_factor,
                                  int vertex_dimension,
                                  int hyperedge_dimension,
                                  const char* hypergraph_file,
                                  const char* fixed_file,
                                  const char* group_file,
                                  const char* solution_file,
                                  // weight parameters
                                  const std::vector<float>& e_wt_factors,
                                  const std::vector<float>& v_wt_factors);

  // Top level interface
  // The function for partitioning a hypergraph
  // This is the main API for TritonPart
  // Key supports:
  // (1) fixed vertices constraint in fixed_file
  // (2) community attributes in community_file (This can be used to guide the
  // partitioning process) (3) stay together attributes in group_file. (4)
  // timing-driven partitioning (5) fence-aware partitioning (6) placement-aware
  // partitioning, placement information is extracted from OpenDB
  void tritonPartDesign(unsigned int num_parts_arg,
                        float balance_constraint_arg,
                        const std::vector<float>& base_balance_arg,
                        const std::vector<float>& scale_factor_arg,
                        unsigned int seed_arg,
                        bool timing_aware_flag_arg,
                        int top_n_arg,
                        bool placement_flag_arg,
                        bool fence_flag_arg,
                        float fence_lx_arg,
                        float fence_ly_arg,
                        float fence_ux_arg,
                        float fence_uy_arg,
                        const char* fixed_file_arg,
                        const char* community_file_arg,
                        const char* group_file_arg,
                        const char* solution_filename_arg,
                        // timing related parameters
                        float net_timing_factor,
                        float path_timing_factor,
                        float path_snaking_factor,
                        float timing_exp_factor,
                        float extra_delay,
                        bool guardband_flag,
                        // weight parameters
                        const std::vector<float>& e_wt_factors,
                        const std::vector<float>& v_wt_factors,
                        const std::vector<float>& placement_wt_factors,
                        // coarsening related parameters
                        int thr_coarsen_hyperedge_size_skip,
                        int thr_coarsen_vertices,
                        int thr_coarsen_hyperedges,
                        float coarsening_ratio,
                        int max_coarsen_iters,
                        float adj_diff_ratio,
                        int min_num_vertices_each_part,
                        // initial partitioning related parameters
                        int num_initial_solutions,
                        int num_best_initial_solutions,
                        // refinement related parameters
                        int refiner_iters,
                        int max_moves,
                        float early_stop_ratio,
                        int total_corking_passes,
                        // vcycle related parameters
                        bool v_cycle_flag,
                        int max_num_vcycle,
                        int num_coarsen_solutions,
                        int num_vertices_threshold_ilp,
                        int global_net_threshold);

  void evaluatePartDesignSolution(unsigned int num_parts_arg,
                                  float balance_constraint_arg,
                                  const std::vector<float>& base_balance_arg,
                                  const std::vector<float>& scale_factor_arg,
                                  bool timing_aware_flag_arg,
                                  int top_n_arg,
                                  bool fence_flag_arg,
                                  float fence_lx_arg,
                                  float fence_ly_arg,
                                  float fence_ux_arg,
                                  float fence_uy_arg,
                                  const char* fixed_file_arg,
                                  const char* community_file_arg,
                                  const char* group_file_arg,
                                  const char* hypergraph_file_arg,
                                  const char* hypergraph_int_weight_file_arg,
                                  const char* solution_filename_arg,
                                  // timing related parameters
                                  float net_timing_factor,
                                  float path_timing_factor,
                                  float path_snaking_factor,
                                  float timing_exp_factor,
                                  float extra_delay,
                                  bool guardband_flag,
                                  // weight parameters
                                  const std::vector<float>& e_wt_factors,
                                  const std::vector<float>& v_wt_factors);

  // k-way partitioning used by Hier-RTLMP
  std::vector<int> PartitionKWaySimpleMode(
      unsigned int num_parts_arg,
      float balance_constraint_arg,
      unsigned int seed_arg,
      const std::vector<std::vector<int>>& hyperedges,
      const std::vector<float>& vertex_weights,
      const std::vector<float>& hyperedge_weights);

  void readPartitioningFile(const std::string& filename,
                            const std::string& instance_map_file);
  void writePartitionVerilog(const char* file_name,
                             const char* port_prefix = "partition_",
                             const char* module_suffix = "_partition");

 private:
  odb::dbBlock* getDbBlock() const;
  sta::Instance* buildPartitionedInstance(
      const char* name,
      const char* port_prefix,
      sta::Library* library,
      sta::NetworkReader* network,
      sta::Instance* parent,
      const std::set<sta::Instance*, CompareInstancePtr>* insts,
      std::map<sta::Net*, sta::Port*>* port_map);

  sta::Instance* buildPartitionedTopInstance(const char* name,
                                             sta::Library* library,
                                             sta::NetworkReader* network);

  odb::dbDatabase* db_ = nullptr;
  sta::dbNetwork* db_network_ = nullptr;
  sta::dbSta* sta_ = nullptr;
  utl::Logger* logger_ = nullptr;
};

}  // namespace par

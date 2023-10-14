/////////////////////////////////////////////////////////////////////////////
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

%{
#include <array>
#include <iostream>
#include <memory>
#include <regex>
#include <vector>

#include "par/PartitionMgr.h"

namespace ord {
// Defined in OpenRoad.i
par::PartitionMgr* getPartitionMgr();
}  // namespace ord

using ord::getPartitionMgr;
using std::regex;

// From tcl/std_vector.i - not sure why it isn't found automatically
template <typename Type>
int SwigDouble_As(Tcl_Interp * interp, Tcl_Obj * o, Type * val)
{
  int return_val;
  double temp_val;
  return_val = Tcl_GetDoubleFromObj(interp, o, &temp_val);
  *val = (Type) temp_val;
  return return_val;
}

%}

%import<std_vector.i>
%template(wt_factors) std::vector<float>;

%include "../../Exception.i"

%inline %{
void triton_part_hypergraph(unsigned int num_parts,
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
                            int global_net_threshold)
{
  getPartitionMgr()->tritonPartHypergraph(
      num_parts,
      balance_constraint,
      base_balance,
      scale_factor,
      seed,
      vertex_dimension,
      hyperedge_dimension,
      placement_dimension,
      hypergraph_file,
      fixed_file,
      community_file,
      group_file,
      placement_file,
      // weight parameters
      e_wt_factors,
      v_wt_factors,
      placement_wt_factors,
      // coarsening related parameters
      thr_coarsen_hyperedge_size_skip,
      thr_coarsen_vertices,
      thr_coarsen_hyperedges,
      coarsening_ratio,
      max_coarsen_iters,
      adj_diff_ratio,
      min_num_vertices_each_part,
      // initial partitioning related parameters
      num_initial_solutions,
      num_best_initial_solutions,
      // refinement related parameters
      refiner_iters,
      max_moves,
      early_stop_ratio,
      total_corking_passes,
      // vcycle related parameters
      v_cycle_flag,
      max_num_vcycle,
      num_coarsen_solutions,
      num_vertices_threshold_ilp,
      global_net_threshold);
}

void evaluate_hypergraph_solution(unsigned int num_parts,
                                  float balance_constraint,
                                  const std::vector<float>& base_balance,
                                  const std::vector<float>& scale_factor,
                                  int vertex_dimension,
                                  int hyperedge_dimension,
                                  const char* hypergraph_file,
                                  const char* fixed_file,
                                  const char* group_file,
                                  const char* solution_file,
                                  const std::vector<float>& e_wt_factors,
                                  const std::vector<float>& v_wt_factors)
{
  getPartitionMgr()->evaluateHypergraphSolution(num_parts,
                                                balance_constraint,
                                                base_balance,
                                                scale_factor,
                                                vertex_dimension,
                                                hyperedge_dimension,
                                                hypergraph_file,
                                                fixed_file,
                                                group_file,
                                                solution_file,
                                                e_wt_factors,
                                                v_wt_factors);
}

void triton_part_design(unsigned int num_parts_arg,
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
                        int global_net_threshold)
{
  getPartitionMgr()->tritonPartDesign(
      num_parts_arg,
      balance_constraint_arg,
      base_balance_arg,
      scale_factor_arg,
      seed_arg,
      timing_aware_flag_arg,
      top_n_arg,
      placement_flag_arg,
      fence_flag_arg,
      fence_lx_arg,
      fence_ly_arg,
      fence_ux_arg,
      fence_uy_arg,
      fixed_file_arg,
      community_file_arg,
      group_file_arg,
      solution_filename_arg,
      // timing related parameters
      net_timing_factor,
      path_timing_factor,
      path_snaking_factor,
      timing_exp_factor,
      extra_delay,
      guardband_flag,
      // weight parameters
      e_wt_factors,
      v_wt_factors,
      placement_wt_factors,
      // coarsening related parameters
      thr_coarsen_hyperedge_size_skip,
      thr_coarsen_vertices,
      thr_coarsen_hyperedges,
      coarsening_ratio,
      max_coarsen_iters,
      adj_diff_ratio,
      min_num_vertices_each_part,
      // initial partitioning related parameters
      num_initial_solutions,
      num_best_initial_solutions,
      // refinement related parameters
      refiner_iters,
      max_moves,
      early_stop_ratio,
      total_corking_passes,
      // vcycle related parameters
      v_cycle_flag,
      max_num_vcycle,
      num_coarsen_solutions,
      num_vertices_threshold_ilp,
      global_net_threshold);
}

void evaluate_part_design_solution(unsigned int num_parts_arg,
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
                                   const std::vector<float>& v_wt_factors)
{
  getPartitionMgr()->evaluatePartDesignSolution(
      num_parts_arg,
      balance_constraint_arg,
      base_balance_arg,
      scale_factor_arg,
      timing_aware_flag_arg,
      top_n_arg,
      fence_flag_arg,
      fence_lx_arg,
      fence_ly_arg,
      fence_ux_arg,
      fence_uy_arg,
      fixed_file_arg,
      community_file_arg,
      group_file_arg,
      hypergraph_file_arg,
      hypergraph_int_weight_file_arg,
      solution_filename_arg,
      // timing related parameters
      net_timing_factor,
      path_timing_factor,
      path_snaking_factor,
      timing_exp_factor,
      extra_delay,
      guardband_flag,
      // weight parameters
      e_wt_factors,
      v_wt_factors);
}

void write_partition_verilog(
    const char* port_prefix, const char* module_suffix, const char* file_name)
{
  getPartitionMgr()->writePartitionVerilog(
      file_name, port_prefix, module_suffix);
}

void read_file(const char* filename, const char* instance_map_file)
{
  getPartitionMgr()->readPartitioningFile(filename, instance_map_file);
}

%}

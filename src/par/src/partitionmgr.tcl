###############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, The Regents of the University of California
## All rights reserved.
##
## Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are met:
##
## * Redistributions of source code must retain the above copyright notice, this
##   list of conditions and the following disclaimer.
##
## * Redistributions in binary form must reproduce the above copyright notice,
##   this list of conditions and the following disclaimer in the documentation
##   and#or other materials provided with the distribution.
##
## * Neither the name of the copyright holder nor the names of its
##   contributors may be used to endorse or promote products derived from
##   this software without specific prior written permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
## AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
## IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
## ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
## LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
## CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
## SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
## INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
## CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
## ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
## POSSIBILITY OF SUCH DAMAGE.
##
################################################################################

#--------------------------------------------------------------------
# Partition netlist command
#--------------------------------------------------------------------
sta::define_cmd_args "triton_part_hypergraph" {
  -hypergraph_file hypergraph_file \
  -num_parts num_parts \
  -balance_constraint balance_constraint \
  [-base_balance base_balance] \
  [-scale_factor scale_factor] \
  [-seed seed] \
  [-vertex_dimension vertex_dimension] \
  [-hyperedge_dimension hyperedge_dimension] \
  [-placement_dimension placement_dimension] \
  [-fixed_file fixed_file] \
  [-community_file community_file] \
  [-group_file group_file] \
  [-placement_file placement_file] \
  [-e_wt_factors e_wt_factors] \
  [-v_wt_factors <v_wt_factors>] \
  [-placement_wt_factors <placement_wt_factors>] \
  [-thr_coarsen_hyperedge_size_skip thr_coarsen_hyperedge_size_skip] \
  [-thr_coarsen_vertices thr_coarsen_vertices] \
  [-thr_coarsen_hyperedges thr_coarsen_hyperedges] \
  [-coarsening_ratio coarsening_ratio] \
  [-max_coarsen_iters max_coarsen_iters] \
  [-adj_diff_ratio adj_diff_ratio] \
  [-min_num_vertices_each_part min_num_vertices_each_part] \
  [-num_initial_solutions num_initial_solutions] \
  [-num_best_initial_solutions num_best_initial_solutions] \
  [-refiner_iters refiner_iters] \
  [-max_moves max_moves] \
  [-early_stop_ratio early_stop_ratio] \
  [-total_corking_passes total_corking_passes] \
  [-v_cycle_flag v_cycle_flag ] \
  [-max_num_vcycle max_num_vcycle] \
  [-num_coarsen_solutions num_coarsen_solutions] \
  [-num_vertices_threshold_ilp num_vertices_threshold_ilp] \
  [-global_net_threshold global_net_threshold] \
  }
proc triton_part_hypergraph { args } {
  sta::parse_key_args "triton_part_hypergraph" args \
    keys {-num_parts \
          -balance_constraint \
          -base_balance \
          -scale_factor \
          -seed \
          -vertex_dimension \
          -hyperedge_dimension \
          -placement_dimension \
          -hypergraph_file \
          -fixed_file \
          -community_file \
          -group_file \
          -placement_file \
          -e_wt_factors \
          -v_wt_factors \
          -placement_wt_factors \
          -thr_coarsen_hyperedge_size_skip \
          -thr_coarsen_vertices \
          -thr_coarsen_hyperedges \
          -coarsening_ratio \
          -max_coarsen_iters \
          -adj_diff_ratio \
          -min_num_vertices_each_part \
          -num_initial_solutions \
          -num_best_initial_solutions \
          -refiner_iters \
          -max_moves \
          -early_stop_ratio \
          -total_corking_passes \
          -v_cycle_flag \
          -max_num_vcycle \
          -num_coarsen_solutions \
          -num_vertices_threshold_ilp \
          -global_net_threshold } \
    flags {}

  if { ![info exists keys(-hypergraph_file)] } {
    utl::error PAR 0924 "Missing mandatory argument -hypergraph_file."
  }
  set hypergraph_file $keys(-hypergraph_file)
  set num_parts 2
  set balance_constraint 1.0
  set base_balance { 1.0 }
  set scale_factor { 1.0 }
  set seed 0
  set vertex_dimension 1
  set hyperedge_dimension 1
  set placement_dimension 0
  set fixed_file ""
  set community_file ""
  set group_file ""
  set placement_file ""
  set e_wt_factors { 1.0 }
  set v_wt_factors { 1.0 }
  set placement_wt_factors { 1.0 }
  set thr_coarsen_hyperedge_size_skip 200
  set thr_coarsen_vertices 10
  set thr_coarsen_hyperedges 50
  set coarsening_ratio 1.6
  set max_coarsen_iters 30
  set adj_diff_ratio 0.0001
  set min_num_vertices_each_part 4
  set num_initial_solutions 50
  set num_best_initial_solutions 10
  set refiner_iters 10
  set max_moves 60
  set early_stop_ratio 0.5
  set total_corking_passes 25
  set v_cycle_flag true
  set max_num_vcycle 1
  set num_coarsen_solutions 3
  set num_vertices_threshold_ilp 50
  set global_net_threshold 1000

  if { [info exists keys(-num_parts)] } {
    set num_parts $keys(-num_parts)
  }

  if { [info exists keys(-balance_constraint)] } {
    set balance_constraint $keys(-balance_constraint)
  }

  if { [info exists keys(-seed)] } {
    set seed $keys(-seed)
  }

  if { [info exists keys(-vertex_dimension)] } {
    set vertex_dimension $keys(-vertex_dimension)
  }

  if { [info exists keys(-hyperedge_dimension)] } {
    set hyperedge_dimension $keys(-hyperedge_dimension)
  }

  if { [info exists keys(-placement_dimension)] } {
    set placement_dimension $keys(-placement_dimension)
  }

  if { [info exists keys(-fixed_file)] } {
    set fixed_file $keys(-fixed_file)
  }

  if { [info exists keys(-community_file)] } {
    set community_file $keys(-community_file)
  }

  if { [info exists keys(-group_file)] } {
    set group_file $keys(-group_file)
  }

  if { [info exists keys(-placement_file)] } {
    set placement_file $keys(-placement_file)
  }

  if { [info exists keys(-base_balance)] } {
    set base_balance $keys(-base_balance)
  }

  if { [info exists keys(-scale_factor)] } {
    set scale_factor $keys(-scale_factor)
  }

  if { [info exists keys(-e_wt_factors)] } {
    set e_wt_factors $keys(-e_wt_factors)
  }

  if { [info exists keys(-v_wt_factors)] } {
    set v_wt_factors $keys(-v_wt_factors)
  }

  if { [info exists keys(-placement_wt_factors)] } {
    set placement_wt_factors $keys(-placement_wt_factors)
  }

  if { [info exists keys(-thr_coarsen_hyperedge_size_skip)] } {
    set thr_coarsen_hyperedge_size_skip $keys(-thr_coarsen_hyperedge_size_skip)
  }

  if { [info exists keys(-thr_coarsen_vertices)] } {
    set thr_coarsen_vertices $keys(-thr_coarsen_vertices)
  }

  if { [info exists keys(-thr_coarsen_hyperedges)] } {
    set thr_coarsen_hyperedges $keys(-thr_coarsen_hyperedges)
  }

  if { [info exists keys(-coarsening_ratio)] } {
    set coarsening_ratio $keys(-coarsening_ratio)
  }

  if { [info exists keys(-max_coarsen_iters)] } {
    set max_coarsen_iters $keys(-max_coarsen_iters)
  }

  if { [info exists keys(-adj_diff_ratio)] } {
    set adj_diff_ratio $keys(-adj_diff_ratio)
  }

  if { [info exists keys(-min_num_vertices_each_part)] } {
    set min_num_vertices_each_part $keys(-min_num_vertices_each_part)
  }

  if { [info exists keys(-num_initial_solutions)] } {
    set num_initial_solutions $keys(-num_initial_solutions)
  }

  if { [info exists keys(-num_best_initial_solutions)] } {
    set num_best_initial_solutions $keys(-num_best_initial_solutions)
  }

  if { [info exists keys(-refiner_iters)] } {
    set refiner_iters $keys(-refiner_iters)
  }

  if { [info exists keys(-max_moves)] } {
    set max_moves $keys(-max_moves)
  }

  if { [info exists keys(-early_stop_ratio)] } {
    set early_stop_ratio $keys(-early_stop_ratio)
  }

  if { [info exists keys(-total_corking_passes)] } {
    set total_corking_passes $keys(-total_corking_passes)
  }

  if { [info exists keys(-v_cycle_flag)] } {
    set v_cycle_flag $keys(-v_cycle_flag)
  }

  if { [info exists keys(-max_num_vcycle)] } {
    set max_num_vcycle $keys(-max_num_vcycle)
  }

  if { [info exists keys(-num_coarsen_solutions)] } {
    set num_coarsen_solutions $keys(-num_coarsen_solutions)
  }

  if { [info exists keys(-num_vertices_threshold_ilp)] } {
    set num_vertices_threshold_ilp $keys(-num_vertices_threshold_ilp)
  }

  if { [info exists keys(-global_net_threshold)] } {
    set global_net_threshold $keys(-global_net_threshold)
  }

  par::triton_part_hypergraph $num_parts \
    $balance_constraint \
    $base_balance \
    $scale_factor \
    $seed \
    $vertex_dimension \
    $hyperedge_dimension \
    $placement_dimension \
    $hypergraph_file \
    $fixed_file \
    $community_file \
    $group_file \
    $placement_file \
    $e_wt_factors \
    $v_wt_factors \
    $placement_wt_factors \
    $thr_coarsen_hyperedge_size_skip \
    $thr_coarsen_vertices \
    $thr_coarsen_hyperedges \
    $coarsening_ratio \
    $max_coarsen_iters \
    $adj_diff_ratio \
    $min_num_vertices_each_part \
    $num_initial_solutions \
    $num_best_initial_solutions \
    $refiner_iters \
    $max_moves \
    $early_stop_ratio \
    $total_corking_passes \
    $v_cycle_flag \
    $max_num_vcycle \
    $num_coarsen_solutions \
    $num_vertices_threshold_ilp \
    $global_net_threshold
}

sta::define_cmd_args "evaluate_hypergraph_solution" {
  -num_parts num_parts \
  -balance_constraint balance_constraint \
  -hypergraph_file hypergraph_file \
  -solution_file solution_file \
  [-base_balance base_balance] \
  [-scale_factor scale_factor] \
  [-vertex_dimension vertex_dimension] \
  [-hyperedge_dimension hyperedge_dimension] \
  [-fixed_file fixed_file] \
  [-group_file group_file] \
  [-e_wt_factors e_wt_factors] \
  [-v_wt_factors v_wt_factors] \
  }
proc evaluate_hypergraph_solution { args } {
  sta::parse_key_args "evaluate_hypergraph_solution" args \
    keys {-num_parts \
          -balance_constraint \
          -base_balance \
          -scale_factor \
          -vertex_dimension \
          -hyperedge_dimension \
          -hypergraph_file \
          -solution_file \
          -fixed_file \
          -group_file \
          -e_wt_factors \
          -v_wt_factors \
           } \
    flags {}
  if { ![info exists keys(-hypergraph_file)] } {
    utl::error PAR 0925 "Missing mandatory argument -hypergraph_file."
  }
  set hypergraph_file $keys(-hypergraph_file)
  set solution_file $keys(-solution_file)
  set num_parts 2
  set base_balance { 1.0 }
  set scale_factor { 1.0 }
  set balance_constraint 1.0
  set vertex_dimension 1
  set hyperedge_dimension 1
  set fixed_file ""
  set group_file ""
  set e_wt_factors { 1.0 }
  set v_wt_factors { 1.0 }

  if { [info exists keys(-num_parts)] } {
    set num_parts $keys(-num_parts)
  }

  if { [info exists keys(-balance_constraint)] } {
    set balance_constraint $keys(-balance_constraint)
  }

  if { [info exists keys(-seed)] } {
    set seed $keys(-seed)
  }

  if { [info exists keys(-vertex_dimension)] } {
    set vertex_dimension $keys(-vertex_dimension)
  }

  if { [info exists keys(-hyperedge_dimension)] } {
    set hyperedge_dimension $keys(-hyperedge_dimension)
  }

  if { [info exists keys(-fixed_file)] } {
    set fixed_file $keys(-fixed_file)
  }

  if { [info exists keys(-group_file)] } {
    set group_file $keys(-group_file)
  }

  if { [info exists keys(-base_balance)] } {
    set base_balance $keys(-base_balance)
  }

  if { [info exists keys(-scale_factor)] } {
    set scale_factor $keys(-scale_factor)
  }

  if { [info exists keys(-e_wt_factors)] } {
    set e_wt_factors $keys(-e_wt_factors)
  }

  if { [info exists keys(-v_wt_factors)] } {
    set v_wt_factors $keys(-v_wt_factors)
  }

  par::evaluate_hypergraph_solution $num_parts \
    $balance_constraint \
    $base_balance \
    $scale_factor \
    $vertex_dimension \
    $hyperedge_dimension \
    $hypergraph_file \
    $fixed_file \
    $group_file \
    $solution_file \
    $e_wt_factors \
    $v_wt_factors
}


sta::define_cmd_args "triton_part_design" { \
    [-num_parts num_parts] \
    [-balance_constraint balance_constraint] \
    [-base_balance base_balance] \
    [-scale_factor scale_factor] \
    [-seed seed] \
    [-timing_aware_flag timing_aware_flag] \
    [-top_n top_n] \
    [-placement_flag placement_flag] \
    [-fence_flag fence_flag] \
    [-fence_lx fence_lx] \
    [-fence_ly fence_ly] \
    [-fence_ux fence_ux] \
    [-fence_uy fence_uy] \
    [-fixed_file fixed_file] \
    [-community_file community_file] \
    [-group_file group_file] \
    [-solution_file solution_file] \
    [-net_timing_factor net_timing_factor] \
    [-path_timing_factor path_timing_factor] \
    [-path_snaking_factor path_snaking_factor] \
    [-timing_exp_factor timing_exp_factor] \
    [-extra_delay extra_delay] \
    [-guardband_flag guardband_flag] \
    [-e_wt_factors e_wt_factors] \
    [-v_wt_factors v_wt_factors] \
    [-placement_wt_factors placement_wt_factors] \
    [-thr_coarsen_hyperedge_size_skip thr_coarsen_hyperedge_size_skip] \
    [-thr_coarsen_vertices thr_coarsen_vertices] \
    [-thr_coarsen_hyperedges thr_coarsen_hyperedges] \
    [-coarsening_ratio coarsening_ratio] \
    [-max_coarsen_iters max_coarsen_iters] \
    [-adj_diff_ratio adj_diff_ratio] \
    [-min_num_vertices_each_part min_num_vertices_each_part] \
    [-num_initial_solutions num_initial_solutions] \
    [-num_best_initial_solutions num_best_initial_solutions] \
    [-refiner_iters refiner_iters] \
    [-max_moves max_moves] \
    [-early_stop_ratio early_stop_ratio] \
    [-total_corking_passes total_corking_passes] \
    [-v_cycle_flag v_cycle_flag ] \
    [-max_num_vcycle max_num_vcycle] \
    [-num_coarsen_solutions num_coarsen_solutions] \
    [-num_vertices_threshold_ilp num_vertices_threshold_ilp] \
    [-global_net_threshold global_net_threshold] \
}
proc triton_part_design { args } {
  sta::parse_key_args "triton_part_design" args \
    keys {-num_parts \
          -balance_constraint \
          -base_balance \
          -scale_factor \
          -seed \
          -timing_aware_flag \
          -top_n \
          -placement_flag \
          -fence_flag  \
          -fence_lx  \
          -fence_ly  \
          -fence_ux  \
          -fence_uy  \
          -fixed_file \
          -community_file \
          -group_file \
          -solution_file \
          -net_timing_factor \
          -path_timing_factor \
          -path_snaking_factor \
          -timing_exp_factor \
          -extra_delay \
          -guardband_flag \
          -e_wt_factors \
          -v_wt_factors \
          -placement_wt_factors \
          -thr_coarsen_hyperedge_size_skip \
          -thr_coarsen_vertices \
          -thr_coarsen_hyperedges \
          -coarsening_ratio \
          -max_coarsen_iters \
          -adj_diff_ratio \
          -min_num_vertices_each_part \
          -num_initial_solutions \
          -num_best_initial_solutions \
          -refiner_iters \
          -max_moves \
          -early_stop_ratio \
          -total_corking_passes \
          -v_cycle_flag \
          -max_num_vcycle \
          -num_coarsen_solutions \
          -num_vertices_threshold_ilp \
          -global_net_threshold } \
    flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error PAR 103 "No design block found."
  }

  set num_parts 2
  set balance_constraint 1.0
  set base_balance { 1.0 }
  set scale_factor { 1.0 }
  set seed 1
  set timing_aware_flag true
  set top_n 1000
  set placement_flag false
  set fence_flag false
  set fence_lx 0.0
  set fence_ly 0.0
  set fence_ux 0.0
  set fence_uy 0.0
  set fixed_file ""
  set community_file ""
  set group_file ""
  set solution_file ""
  set net_timing_factor 1.0
  set path_timing_factor 1.0
  set path_snaking_factor 1.0
  set timing_exp_factor 1.0
  set extra_delay 1e-9
  set guardband_flag false
  set e_wt_factors { 1.0 }
  set v_wt_factors { 1.0 }
  set placement_wt_factors { }
  set thr_coarsen_hyperedge_size_skip 1000
  set thr_coarsen_vertices 10
  set thr_coarsen_hyperedges 50
  set coarsening_ratio 1.5
  set max_coarsen_iters 30
  set adj_diff_ratio 0.0001
  set min_num_vertices_each_part 4
  set num_initial_solutions 100
  set num_best_initial_solutions 10
  set refiner_iters 10
  set max_moves 100
  set early_stop_ratio 0.5
  set total_corking_passes 25
  set v_cycle_flag true
  set max_num_vcycle 1
  set num_coarsen_solutions 4
  set num_vertices_threshold_ilp 50
  set global_net_threshold 1000

  if { [info exists keys(-num_parts)] } {
    set num_parts $keys(-num_parts)
  }

  if { [info exists keys(-base_balance)] } {
    set base_balance $keys(-base_balance)
  }

  if { [info exists keys(-scale_factor)] } {
    set scale_factor $keys(-scale_factor)
  }

  if { [info exists keys(-balance_constraint)] } {
    set balance_constraint $keys(-balance_constraint)
  }

  if { [info exists keys(-seed)] } {
    set seed $keys(-seed)
  }

  if { [info exists keys(-timing_aware_flag)] } {
    set timing_aware_flag $keys(-timing_aware_flag)
  }

  if { [info exists keys(-top_n)] } {
    set top_n $keys(-top_n)
  }

  if { [info exists keys(-placement_flag)] } {
    set placement_flag $keys(-placement_flag)
  }

  if {
    [info exists keys(-fence_flag)] &&
    [info exists keys(-fence_lx)] &&
    [info exists keys(-fence_ly)] &&
    [info exists keys(-fence_ux)] &&
    [info exists keys(-fence_uy)]
  } {
    set fence_flag $keys(-fence_flag)
    set fence_lx $keys(-fence_lx)
    set fence_ly $keys(-fence_ly)
    set fence_ux $keys(-fence_ux)
    set fence_uy $keys(-fence_uy)
  }

  if { [info exists keys(-fixed_file)] } {
    set fixed_file $keys(-fixed_file)
  }

  if { [info exists keys(-community_file)] } {
    set community_file $keys(-community_file)
  }

  if { [info exists keys(-group_file)] } {
    set group_file $keys(-group_file)
  }

  if { [info exists keys(-solution_file)] } {
    set solution_file $keys(-solution_file)
  }

  if { [info exists keys(-net_timing_factor)] } {
    set net_timing_factor $keys(-net_timing_factor)
  }

  if { [info exists keys(-path_timing_factor)] } {
    set path_timing_factor $keys(-path_timing_factor)
  }

  if { [info exists keys(-path_snaking_factor)] } {
    set path_snaking_factor $keys(-path_snaking_factor)
  }

  if { [info exists keys(-timing_exp_factor)] } {
    set timing_exp_factor $keys(-timing_exp_factor)
  }

  if { [info exists keys(-extra_delay)] } {
    set extra_delay $keys(-extra_delay)
  }

  if { [info exists keys(-guardband_flag)] } {
    set guardband_flag $keys(-guardband_flag)
  }

  if { [info exists keys(-e_wt_factors)] } {
    set e_wt_factors $keys(-e_wt_factors)
  }

  if { [info exists keys(-v_wt_factors)] } {
    set v_wt_factors $keys(-v_wt_factors)
  }

  if { [info exists keys(-placement_wt_factors)] } {
    set placement_wt_factors $keys(-placement_wt_factors)
  }

  if { [info exists keys(-thr_coarsen_hyperedge_size_skip)] } {
    set thr_coarsen_hyperedge_size_skip $keys(-thr_coarsen_hyperedge_size_skip)
  }

  if { [info exists keys(-thr_coarsen_vertices)] } {
    set thr_coarsen_vertices $keys(-thr_coarsen_vertices)
  }

  if { [info exists keys(-thr_coarsen_hyperedges)] } {
    set thr_coarsen_hyperedges $keys(-thr_coarsen_hyperedges)
  }

  if { [info exists keys(-coarsening_ratio)] } {
    set coarsening_ratio $keys(-coarsening_ratio)
  }

  if { [info exists keys(-max_coarsen_iters)] } {
    set max_coarsen_iters $keys(-max_coarsen_iters)
  }

  if { [info exists keys(-adj_diff_ratio)] } {
    set adj_diff_ratio $keys(-adj_diff_ratio)
  }

  if { [info exists keys(-min_num_vertices_each_part)] } {
    set min_num_vertices_each_part $keys(-min_num_vertices_each_part)
  }

  if { [info exists keys(-num_initial_solutions)] } {
    set num_initial_solutions $keys(-num_initial_solutions)
  }

  if { [info exists keys(-num_best_initial_solutions)] } {
    set num_best_initial_solutions $keys(-num_best_initial_solutions)
  }

  if { [info exists keys(-refiner_iters)] } {
    set refiner_iters $keys(-refiner_iters)
  }

  if { [info exists keys(-max_moves)] } {
    set max_moves $keys(-max_moves)
  }

  if { [info exists keys(-early_stop_ratio)] } {
    set early_stop_ratio $keys(-early_stop_ratio)
  }

  if { [info exists keys(-total_corking_passes)] } {
    set total_corking_passes $keys(-total_corking_passes)
  }

  if { [info exists keys(-v_cycle_flag)] } {
    set v_cycle_flag $keys(-v_cycle_flag)
  }

  if { [info exists keys(-max_num_vcycle)] } {
    set max_num_vcycle $keys(-max_num_vcycle)
  }

  if { [info exists keys(-num_coarsen_solutions)] } {
    set num_coarsen_solutions $keys(-num_coarsen_solutions)
  }

  if { [info exists keys(-num_vertices_threshold_ilp)] } {
    set num_vertices_threshold_ilp $keys(-num_vertices_threshold_ilp)
  }

  if { [info exists keys(-global_net_threshold)] } {
    set global_net_threshold $keys(-global_net_threshold)
  }

  par::triton_part_design $num_parts \
    $balance_constraint \
    $base_balance \
    $scale_factor \
    $seed \
    $timing_aware_flag \
    $top_n \
    $placement_flag \
    $fence_flag \
    $fence_lx \
    $fence_ly \
    $fence_ux \
    $fence_uy \
    $fixed_file \
    $community_file \
    $group_file \
    $solution_file \
    $net_timing_factor \
    $path_timing_factor \
    $path_snaking_factor \
    $timing_exp_factor \
    $extra_delay \
    $guardband_flag \
    $e_wt_factors \
    $v_wt_factors \
    $placement_wt_factors \
    $thr_coarsen_hyperedge_size_skip \
    $thr_coarsen_vertices \
    $thr_coarsen_hyperedges \
    $coarsening_ratio \
    $max_coarsen_iters \
    $adj_diff_ratio \
    $min_num_vertices_each_part \
    $num_initial_solutions \
    $num_best_initial_solutions \
    $refiner_iters \
    $max_moves \
    $early_stop_ratio \
    $total_corking_passes \
    $v_cycle_flag \
    $max_num_vcycle \
    $num_coarsen_solutions \
    $num_vertices_threshold_ilp \
    $global_net_threshold
}


sta::define_cmd_args "evaluate_part_design_solution" {
  [-num_parts num_parts] \
  [-balance_constraint balance_constraint] \
  [-base_balance base_balance] \
  [-scale_factor scale_factor] \
  [-timing_aware_flag timing_aware_flag] \
  [-top_n top_n] \
  [-fence_flag fence_flag] \
  [-fence_lx fence_lx] \
  [-fence_ly fence_ly] \
  [-fence_ux fence_ux] \
  [-fence_uy fence_uy] \
  [-fixed_file fixed_file] \
  [-community_file community_file] \
  [-group_file group_file] \
  [-hypergraph_file hypergraph_file]
  [-hypergraph_int_weight_file hypergraph_int_weight_file]
  [-solution_file solution_file] \
  [-net_timing_factor net_timing_factor] \
  [-path_timing_factor path_timing_factor] \
  [-path_snaking_factor path_snaking_factor] \
  [-timing_exp_factor timing_exp_factor] \
  [-extra_delay extra_delay] \
  [-guardband_flag guardband_flag] \
  [-e_wt_factors e_wt_factors] \
  [-v_wt_factors v_wt_factors] }
proc evaluate_part_design_solution { args } {
  sta::parse_key_args "evaluate_part_design_solution" args \
    keys {-num_parts \
          -balance_constraint \
          -base_balance \
          -scale_factor \
          -timing_aware_flag \
          -top_n \
          -fence_flag  \
          -fence_lx  \
          -fence_ly  \
          -fence_ux  \
          -fence_uy  \
          -fixed_file \
          -community_file \
          -group_file \
          -hypergraph_file \
          -hypergraph_int_weight_file \
          -solution_file \
          -net_timing_factor \
          -path_timing_factor \
          -path_snaking_factor \
          -timing_exp_factor \
          -extra_delay \
          -guardband_flag \
          -e_wt_factors \
          -v_wt_factors  } \
    flags {}

  if { [ord::get_db_block] == "NULL" } {
    utl::error PAR 104 "No design block found."
  }

  set num_parts 2
  set balance_constraint 1.0
  set base_balance { 1.0 }
  set scale_factor { 1.0 }
  set timing_aware_flag true
  set top_n 1000
  set fence_flag false
  set fence_lx 0.0
  set fence_ly 0.0
  set fence_ux 0.0
  set fence_uy 0.0
  set fixed_file ""
  set community_file ""
  set group_file ""
  set hypergraph_file ""
  set hypergraph_int_weight_file ""
  set solution_file ""
  set net_timing_factor 1.0
  set path_timing_factor 1.0
  set path_snaking_factor 1.0
  set timing_exp_factor 1.0
  set extra_delay 1e-9
  set e_wt_factors { 1.0 }
  set v_wt_factors { 1.0 }
  # For fair evaluation, guardband_flag should be turned off
  set guardband_flag false

  if { [info exists keys(-num_parts)] } {
    set num_parts $keys(-num_parts)
  }

  if { [info exists keys(-balance_constraint)] } {
    set balance_constraint $keys(-balance_constraint)
  }

  if { [info exists keys(-base_balance)] } {
    set base_balance $keys(-base_balance)
  }

  if { [info exists keys(-scale_factor)] } {
    set scale_factor $keys(-scale_factor)
  }

  if { [info exists keys(-timing_aware_flag)] } {
    set timing_aware_flag $keys(-timing_aware_flag)
  }

  if { [info exists keys(-top_n)] } {
    set top_n $keys(-top_n)
  }

  if {
    [info exists keys(-fence_flag)] &&
    [info exists keys(-fence_lx)] &&
    [info exists keys(-fence_ly)] &&
    [info exists keys(-fence_ux)] &&
    [info exists keys(-fence_uy)]
  } {
    set fence_flag $keys(-fence_flag)
    set fence_lx $keys(-fence_lx)
    set fence_ly $keys(-fence_ly)
    set fence_ux $keys(-fence_ux)
    set fence_uy $keys(-fence_uy)
  }

  if { [info exists keys(-fixed_file)] } {
    set fixed_file $keys(-fixed_file)
  }

  if { [info exists keys(-community_file)] } {
    set community_file $keys(-community_file)
  }

  if { [info exists keys(-group_file)] } {
    set group_file $keys(-group_file)
  }

  if { [info exists keys(-hypergraph_file)] } {
    set hypergraph_file $keys(-hypergraph_file)
  }

  if { [info exists keys(-hypergraph_int_weight_file)] } {
    set hypergraph_int_weight_file $keys(-hypergraph_int_weight_file)
  }

  if { [info exists keys(-solution_file)] } {
    set solution_file $keys(-solution_file)
  }

  if { [info exists keys(-net_timing_factor)] } {
    set net_timing_factor $keys(-net_timing_factor)
  }

  if { [info exists keys(-path_timing_factor)] } {
    set path_timing_factor $keys(-path_timing_factor)
  }

  if { [info exists keys(-path_snaking_factor)] } {
    set path_snaking_factor $keys(-path_snaking_factor)
  }

  if { [info exists keys(-timing_exp_factor)] } {
    set timing_exp_factor $keys(-timing_exp_factor)
  }

  if { [info exists keys(-extra_delay)] } {
    set extra_delay $keys(-extra_delay)
  }

  if { [info exists keys(-guardband_flag)] } {
    set guardband_flag $keys(-guardband_flag)
  }

  if { [info exists keys(-e_wt_factors)] } {
    set e_wt_factors $keys(-e_wt_factors)
  }

  if { [info exists keys(-v_wt_factors)] } {
    set v_wt_factors $keys(-v_wt_factors)
  }

  par::evaluate_part_design_solution $num_parts \
    $balance_constraint \
    $base_balance \
    $scale_factor \
    $timing_aware_flag \
    $top_n \
    $fence_flag \
    $fence_lx \
    $fence_ly \
    $fence_ux \
    $fence_uy \
    $fixed_file \
    $community_file \
    $group_file \
    $hypergraph_file \
    $hypergraph_int_weight_file \
    $solution_file \
    $net_timing_factor \
    $path_timing_factor \
    $path_snaking_factor \
    $timing_exp_factor \
    $extra_delay \
    $guardband_flag \
    $e_wt_factors \
    $v_wt_factors
}


#--------------------------------------------------------------------
# Write partition to verilog
#--------------------------------------------------------------------

sta::define_cmd_args "write_partition_verilog" { \
  [-port_prefix prefix] [-module_suffix suffix] [-partitioning_id id] [file]
}

proc write_partition_verilog { args } {
  sta::parse_key_args "write_partition_verilog" args \
    keys { -partitioning_id -port_prefix -module_suffix } flags { }

  sta::check_argc_eq1 "write_partition_verilog" $args

  set port_prefix "partition_"
  if { [info exists keys(-port_prefix)] } {
    set port_prefix $keys(-port_prefix)
  }

  set module_suffix "_partition"
  if { [info exists keys(-module_suffix)] } {
    set module_suffix $keys(-module_suffix)
  }

  par::write_partition_verilog $port_prefix $module_suffix $args
}

sta::define_cmd_args "read_partitioning" { -read_file name [-instance_map_file file_path] }

proc read_partitioning { args } {
  sta::parse_key_args "read_partitioning" args \
    keys { -read_file \
      -instance_map_file
    } flags { }

  if { ![info exists keys(-read_file)] } {
    utl::error PAR 51 "Missing mandatory argument -read_file"
  }
  set instance_file ""
  if { [info exists keys(-instance_map_file)] } {
    set instance_file $keys(-instance_map_file)
  }
  return [par::read_file $keys(-read_file) $instance_file]
}

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

sta::define_cmd_args "partition_netlist" { [-tool name] \
  [-num_partitions value] \
    [-graph_model name] \
    [-clique_threshold value] \
    [-weight_model name] \
    [-max_edge_weight value] \
    [-max_vertex_weight value] \
    [-num_starts value] \
    [-balance_constraint value] \
    [-coarsening_ratio value] \
    [-coarsening_vertices value] \
    [-enable_term_prop value] \
    [-cut_hop_ratio value] \
    [-architecture value] \
    [-refinement value] \
    [-random_seed value] \
    [-seeds value] \
    [-partition_id value] \
    [-force_graph value] \
}
proc partition_netlist { args } {
  sta::parse_key_args "partition_netlist" args \
    keys {-tool \
      -num_partitions \
        -graph_model \
        -clique_threshold \
        -weight_model \
        -max_edge_weight \
        -max_vertex_weight \
        -num_starts \
        -balance_constraint \
        -coarsening_ratio \
        -coarsening_vertices \
        -enable_term_prop \
        -cut_hop_ratio \ 
        -architecture \
        -refinement \
        -random_seed \
        -seeds \
        -partition_id \
        -force_graph \
    } flags {}

# Tool
  set tools "chaco gpmetis mlpart"
    if { ![info exists keys(-tool)] } {
      utl::error PAR 25 "missing mandatory argument -tool" 
    } elseif { !($keys(-tool) in $tools) } {
      utl::error PAR 26 "invalid tool. Use one of the following: $tools"
    } else {
      par::set_tool $keys(-tool)
    }

# Clique threshold
  if { [info exists keys(-clique_threshold)] } {
    if { !([string is integer $keys(-clique_threshold)] && \
        $keys(-clique_threshold) >= 3 && $keys(-clique_threshold) <= 32768) } {
      utl::error PAR 27 "argument -clique_threshold should be an integer in the range \[3, 32768\]"
    } else {
      par::set_clique_threshold $keys(-clique_threshold)
    }
  } else {
    par::set_clique_threshold 50
  }

# Graph model
  set graph_models "clique star hybrid"
    if { [info exists keys(-graph_model)] } {
      if { $keys(-graph_model) in $graph_models } {
      } else {
        utl::error PAR 28 "invalid graph model. Use one of the following: $graph_models"
      }
      par::set_graph_model $keys(-graph_model)
    } else {
      par::set_graph_model "star"
    }

# Weight model
  if { [info exists keys(-weight_model)] } {
    if { !([string is integer $keys(-weight_model)] && \
        $keys(-weight_model) >= 1 && $keys(-weight_model) <= 7) } {
      utl::error PAR 29 "argument -weight_model should be an integer in the range \[1, 7\]"
    } else {
      par::set_weight_model $keys(-weight_model)
    }     
  } else {
    par::set_weight_model 1
  }

# Max edge weight
  if { [info exists keys(-max_edge_weight)] } {
    if { !([string is integer $keys(-max_edge_weight)] && \
        $keys(-max_edge_weight) >= 1 && $keys(-max_edge_weight) <= 32768) } {
      utl::error PAR 30 "argument -max_edge_weight should be an integer in the range \[1, 32768\]"
    } else {
      par::set_max_edge_weight $keys(-max_edge_weight)
    }       
  } else {
    par::set_max_edge_weight 100
  }

# Max vertex weight
  if { [info exists keys(-max_vertex_weight)] } {
    if { !([string is integer $keys(-max_vertex_weight)] && \
        $keys(-max_vertex_weight) >= 1 && $keys(-max_vertex_weight) <= 32768) } {
      utl::error PAR 31 "argument -max_vertex_weight should be an integer in the range \[1, 32768\]"
    } else {
      par::set_max_vertex_weight $keys(-max_vertex_weight)
    }       
  } else {
    par::set_max_vertex_weight 100
  }

# Num starts
  if { [info exists keys(-num_starts)] } {
    if { !([string is integer $keys(-num_starts)] && \
        $keys(-num_starts) >= 1 && $keys(-num_starts) <= 32768) } {
      utl::error PAR 32 "argument -num_starts should be an integer in the range \[1, 32768\]"
    } else {
      par::set_num_starts $keys(-num_starts)
    }       
  }

# Balance constraint
  if { [info exists keys(-balance_constraint)] } {
    if { !([string is integer $keys(-balance_constraint)] && \
        $keys(-balance_constraint) >= 0 && $keys(-balance_constraint) <= 50) } {
      utl::error PAR 33 "argument -balance_constraint should be an integer in the range \[0, 50\]"
    } else {
      par::set_balance_constraint $keys(-balance_constraint)
    }       
  } else {
    par::set_balance_constraint 2
  }

# Coarsening ratio 
  if { [info exists keys(-coarsening_ratio)] } {
    if { !([string is double $keys(-coarsening_ratio)] && \
        $keys(-coarsening_ratio) >= 0.5 && $keys(-coarsening_ratio) <= 1.0) } {
      utl::error PAR 34 "argument -coarsening_ratio should be a floating number in the range \[0.5, 1.0\]"
    } else {
      par::set_coarsening_ratio $keys(-coarsening_ratio)
    }       
  } else {
    par::set_coarsening_ratio 0.7
  }

# Coarsening vertices
  if { [info exists keys(-coarsening_vertices)] } {
    if { !([string is integer $keys(-coarsening_vertices)]) } {
      utl::error PAR 35 "argument -coarsening_vertices should be an integer"
    } else {
      par::set_coarsening_vertices $keys(-coarsening_vertices)
    }
  } else {
    par::set_coarsening_vertices 2500
  }

# Terminal propagation 
  if { [info exists keys(-enable_term_prop)] } {
    if { !([string is integer $keys(-enable_term_prop)] && \
        $keys(-enable_term_prop) >= 0 && $keys(-enable_term_prop) <= 1) } {
      utl::error PAR 36 "argument -enable_term_prop should be 0 or 1"
    } else {
      par::set_enable_term_prop $keys(-enable_term_prop)
    }
  } else {
    par::set_enable_term_prop 0
  }

# Cut hop ratio 
  if { [info exists keys(-cut_hop_ratio)] } {
    if { !([string is double $keys(-cut_hop_ratio)] && \
        $keys(-cut_hop_ratio) >= 0.5 && $keys(-cut_hop_ratio) <= 1.0) } {
      utk::error PAR 37 "argument -cut_hop_ratio should be a floating number in the range \[0.5, 1.0\]"
    } else {
      par::set_cut_hop_ratio $keys(-cut_hop_ratio)
    }       
  } else {
    par::set_cut_hop_ratio 1.0
  }

# Architecture
  if { [info exists keys(-architecture)] } {
    par::set_architecture $keys(-architecture)
  } else {
    par::clear_architecture
  }

# Refinement
  if { [info exists keys(-refinement)] } {
    if { !([string is integer $keys(-refinement)] && \
        $keys(-refinement) >= 0 && $keys(-refinement) <= 32768) } {
      utl::error PAR 38 "argument -refinement should be an integer in the range \[0, 32768\]"
    } else {
      par::set_refinement $keys(-refinement)
    }
  } else {
    par::set_refinement 0
  }

# Seeds
  if { [info exists keys(-random_seed)] } {
    par::set_random_seed $keys(-random_seed)
  } else {
    par::set_random_seed 42
  }
  if { [info exists keys(-seeds)] } {
    par::set_seeds $keys(-seeds)
  } else {
    if {! [info exists keys(-num_starts)]} {
      utl::error PAR 39 "missing argument -seeds or -num_starts."
    }
    par::generate_seeds $keys(-num_starts)
  }


# Number of partitions
  if { ![info exists keys(-num_partitions)] } {
    utl::error PAR 40 "missing mandatory argument \"-num_partitions \[2, 32768\]\""
  } elseif { !([string is integer $keys(-num_partitions)] && \
      $keys(-num_partitions) >= 2 && $keys(-num_partitions) <= 32768)} {
    utl::error PAR 41 "argument -num_partitions should be an integer in the range \[2, 32768\]"
  } else {
    par::set_target_partitions $keys(-num_partitions) 
      if {[expr !(($keys(-num_partitions) & ($keys(-num_partitions) - 1)) == 0)]} {
        par::set_architecture "1 $keys(-num_partitions)"
      }
  }

# Partition Id (for exisisting partitions)
  if { [info exists keys(-partition_id)] } {
    if { !([string is integer $keys(-partition_id)]) } {
      utl::error PAR 42 "argument -partition_id should be an integer"
    } else {
      par::set_existing_id $keys(-partition_id)
    }
  } else {
    par::set_existing_id -1
  }

  if { [info exists keys(-force_graph)] } {
    par::set_force_graph $keys(-force_graph)
  }

  set currentId [par::run_partitioning]

    return $currentId
}

#--------------------------------------------------------------------
# Evaluate partitioning command
#--------------------------------------------------------------------

sta::define_cmd_args "evaluate_partitioning" { [-partition_ids ids] \
  [-evaluation_function function] \
}
proc evaluate_partitioning { args } {
  sta::parse_key_args "evaluate_partitioning" args \
    keys {-partition_ids \
      -evaluation_function \
    } flags {}

# Partition IDs
  if { [info exists keys(-partition_ids)] } {
    par::set_partition_ids_to_test $keys(-partition_ids)
  } else {
    utl::error PAR 43 "missing argument -partition_ids."
  }

# Evaluation Function
  set functions "terminals hyperedges size area runtime hops"
    if { [info exists keys(-evaluation_function)] } {
      if { !($keys(-evaluation_function) in $functions) } {
        utl::error PAR 44 "invalid function. Use one of the following: $functions"
      }
      par::set_evaluation_function $keys(-evaluation_function)
    }

  set bestId [par::evaluate_partitioning]

    return $bestId
}

#--------------------------------------------------------------------
# Write partition to DB command
#--------------------------------------------------------------------

sta::define_cmd_args "write_partitioning_to_db" { [-partitioning_id id] \
  [-dump_to_file name] \
}

proc write_partitioning_to_db { args } {
  sta::parse_key_args "write_partitioning_to_db" args \
    keys { -partitioning_id \
      -dump_to_file \
    } flags { }

  set partitioning_id 0
    if { ![info exists keys(-partitioning_id)] } {
      utl::error PAR 102 "missing mandatory argument -partitioning_id"
    } else {
      set partition_id $keys(-partitioning_id)
    } 

  par::write_partitioning_to_db $partitioning_id

    if { [info exists keys(-dump_to_file)] } {
      par::dump_part_id_to_file $keys(-dump_to_file)
    } 
}

#--------------------------------------------------------------------
# Write partition to verilog
#--------------------------------------------------------------------

sta::define_cmd_args "write_partition_verilog" { [-partitioning_id id] \
  [-port_prefix prefix] [-module_suffix suffix] [file]
}

proc write_partition_verilog { args } {
  sta::parse_key_args "write_partition_verilog" args \
    keys { -partitioning_id -port_prefix -module_suffix } flags { }

  sta::check_argc_eq1 "write_partition_verilog" $args
  
  if { ![info exists keys(-partitioning_id)] } {
    utl::error PAR 45 "missing mandatory argument -partitioning_id"
  } else {
    set partition_id $keys(-partitioning_id)
  }
  
  set port_prefix "partition_"
  if { [info exists keys(-port_prefix)] } {
    set port_prefix $keys(-port_prefix)
  }
  
  set module_suffix "_partition"
  if { [info exists keys(-module_suffix)] } {
    set module_suffix $keys(-module_suffix)
  }
  
  par::write_partition_verilog $partition_id $port_prefix $module_suffix $args
}

#--------------------------------------------------------------------
# Cluster netlist command
#--------------------------------------------------------------------

sta::define_cmd_args "cluster_netlist" { [-tool name] \
  [-coarsening_ratio value] \
    [-coarsening_vertices value] \
    [-level values] \
}
proc cluster_netlist { args } {
  sta::parse_key_args "cluster_netlist" args \
    keys {-tool \
      -coarsening_ratio \
        -coarsening_vertices \
        -level \
    } flags {}

# Tool
  set tools "chaco gpmetis mlpart"
    if { ![info exists keys(-tool)] } {
      utl::error PAR 46 "missing mandatory argument -tool"
    } elseif { !($keys(-tool) in $tools) } {
      utl::error PAR 47 "invalid tool. Use one of the following: $tools"
    } else {
      par::set_tool $keys(-tool)
    }

# Coarsening ratio 
  if { [info exists keys(-coarsening_ratio)] } {
    if { !([string is double $keys(-coarsening_ratio)] && \
        $keys(-coarsening_ratio) >= 0.5 && $keys(-coarsening_ratio) <= 1.0) } {
      utl::error PAR 48 "argument -coarsening_ratio should be a floating number in the range \[0.5, 1.0\]"
    } else {
      par::set_coarsening_ratio $keys(-coarsening_ratio)
    }       
  }

# Coarsening vertices
  if { [info exists keys(-coarsening_vertices)] } {
    par::set_coarsening_vertices $keys(-coarsening_vertices)
  }

# Levels
  if { [info exists keys(-level)] } {
    par::set_level $keys(-level)
  } else {
    par::set_level 1
  }

  par::generate_seeds 1

    set currentId [par::run_3party_clustering]

    return $currentId
}

#--------------------------------------------------------------------
# Write clustering to DB command
#--------------------------------------------------------------------

sta::define_cmd_args "write_clustering_to_db" { [-clustering_id id] \
  [-dump_to_file name] \
}

proc write_clustering_to_db { args } {
  sta::parse_key_args "write_clustering_to_db" args \
    keys { -clustering_id \
      -dump_to_file \
    } flags { }

  set clustering_id 0
    if { ![info exists keys(-clustering_id)] } {
      utl::error PAR 49 "missing mandatory argument -clustering_id"
    } else {
      set clustering_id $keys(-clustering_id)
    } 

  par::write_clustering_to_db $clustering_id

    if { [info exists keys(-dump_to_file)] } {
      par::dump_clus_id_to_file $keys(-dump_to_file)
    } 
}

#--------------------------------------------------------------------
# Report netlist partitions command
#--------------------------------------------------------------------

sta::define_cmd_args "report_netlist_partitions" { [-partitioning_id id] \
}

proc report_netlist_partitions { args } {
  sta::parse_key_args "report_netlist_partitions" args \
    keys { -partitioning_id \
    } flags { }

  set partitioning_id 0
    if { ![info exists keys(-partitioning_id)] } {
      utl::error PAR 50 "missing mandatory argument -partitioning_id"
    } else {
      set partitioning_id $keys(-partitioning_id)
    } 

  par::report_netlist_partitions $partitioning_id
}

sta::define_cmd_args "read_partitioning" { [-read_file name] \
  [-final_partitions] \
}

proc read_partitioning { args } {
  sta::parse_key_args "read_partitioning" args \
    keys { -read_file \
      -final_partitions \
    } flags { }

  par::set_final_partitions $keys(-final_partitions) 
    if { ![info exists keys(-read_file)] } {
      utl::error PAR 51 "missing mandatory argument -read_file"
    } else {
      par::read_file $keys(-read_file)
    } 


  if { ![info exists keys(-final_partitions)] } {
    utl::error PAR 52 "missing mandatory argument \"-final_partitions \[2, 32768\]\""
  } else {
  }
}

sta::define_cmd_args "run_clustering" { [-scheme name] \
}

proc run_clustering { args } {
  sta::parse_key_args "run_clustering" args \
    keys {-scheme \
    } flags {}

# Tool
  set schemes "hem scheme2 scheme3"
    if { ![info exists keys(-scheme)] } {
      utl::error PAR 53 "missing mandatory argument -scheme"
    } elseif { !($keys(-scheme) in $schemes) } {
      utl::error PAR 54 "invalid scheme. Use one of the following: $schemes"
    } else {
      par::set_clustering_scheme $keys(-scheme)
    }
  par::run_clustering
}

sta::define_cmd_args "report_partition_graph" { [-graph_model name
  -clique_threshold value] \
}

proc report_partition_graph { args } {
  sta::parse_key_args "report_partition_graph" args \
    keys {-graph_model \
      -clique_threshold
    } flags {}

  set graph_models "clique star hybrid"
    if { [info exists keys(-graph_model)] } {
      if { $keys(-graph_model) in $graph_models } {
      } else {
        utl::error PAR 66 "invalid graph model. Use one of the following: $graph_models"
      }
      par::set_graph_model $keys(-graph_model)
    } 

  if { [info exists keys(-clique_threshold)] } {
    if { !([string is integer $keys(-clique_threshold)] && \
        $keys(-clique_threshold) >= 3 && $keys(-clique_threshold) <= 32768) } {
      utl::error PAR 69 "argument -clique_threshold should be an integer in the range \[3, 32768\]"
    } else {
      par::set_clique_threshold $keys(-clique_threshold)
    }
  } else {
    par::set_clique_threshold 50
  }


  par::report_graph
}

sta::define_cmd_args "partition_design" { [-max_num_macro max_num_macro] \
                                          [-min_num_macro min_num_macro] \
                                          [-max_num_inst max_num_inst] \
                                          [-min_num_inst min_num_inst] \
                                          [-net_threshold net_threshold] \
                                          [-virtual_weight virtual_weight] \
                                          [-ignore_net_threshold ignore_net_threshold] \
                                          [-report_directory report_file] \
                                          -report_file report_file \
                                        }
proc partition_design { args } {
    sta::parse_key_args "partition_design" args keys {-max_num_macro -min_num_macro
                     -max_num_inst  -min_num_inst -net_threshold -virtual_weight -ignore_net_threshold -report_directory -report_file} flags {  }
    if { ![info exists keys(-report_file)] } {
        utl::error PAR 70 "missing mandatory argument -report_file"
    }
    set report_file $keys(-report_file)
    set max_num_macro 10
    set min_num_macro 2
    set max_num_inst 0
    set min_num_inst 0
    set net_threshold 0
    set virtual_weight 50
    set ignore_net_threshold 0
    set report_directory "rtl_mp"

    if { [info exists keys(-max_num_macro)] } {
        set max_num_macro $keys(-max_num_macro)
    }

    if { [info exists keys(-min_num_macro)] } {
        set min_num_macro $keys(-min_num_macro)
    }

    if { [info exists keys(-max_num_inst)] } {
        set max_num_inst $keys(-max_num_inst)
    }

    if { [info exists keys(-min_num_inst)] } {
        set min_num_inst $keys(-min_num_inst)
    }

    if { [info exists keys(-net_threshold)] } {
        set net_threshold $keys(-net_threshold)
    }

    if { [info exists keys(-virtual_weight)] } {
        set virtual_weight $keys(-virtual_weight)
    }

    if { [info exists keys(-ignore_net_threshold)] } {
        set net_threshold $keys(-ignore_net_threshold)
    }

    if { [info exists keys(-report_directory)] } {
        set report_directory $keys(-report_directory)
    }

    par::partition_design_cmd $max_num_macro $min_num_macro $max_num_inst $min_num_inst $net_threshold $virtual_weight $ignore_net_threshold $report_directory $report_file
}

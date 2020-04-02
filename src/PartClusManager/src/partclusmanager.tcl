# ////////////////////////////////////////////////////////////////////////////////
# // Authors: Mateus Fogaça, Isadora Oliveira and Marcelo Danigno
# // 
# //          (Advisor: Ricardo Reis and Paulo Butzen)
# //
# // BSD 3-Clause License
# //
# // Copyright (c) 2020, Federal University of Rio Grande do Sul (UFRGS)
# // All rights reserved.
# //
# // Redistribution and use in source and binary forms, with or without
# // modification, are permitted provided that the following conditions are met:
# //
# // * Redistributions of source code must retain the above copyright notice, this
# //   list of conditions and the following disclaimer.
# //
# // * Redistributions in binary form must reproduce the above copyright notice,
# //   this list of conditions and the following disclaimer in the documentation
# //   and/or other materials provided with the distribution.
# //
# // * Neither the name of the copyright holder nor the names of its
# //   contributors may be used to endorse or promote products derived from
# //   this software without specific prior written permission.
# //
# // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# // AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# // ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# // LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# // CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# // SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# // CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# // ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# // POSSIBILITY OF SUCH DAMAGE.
# ////////////////////////////////////////////////////////////////////////////////

#--------------------------------------------------------------------
# Partition netlist command
#--------------------------------------------------------------------

sta::define_cmd_args "partition_netlist" { [-tool name] \
                                           [-target_partitions value] \
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
                                           [-seeds value] \
                                         }
proc partition_netlist { args } {
  sta::parse_key_args "partition_netlist" args \
    keys {-tool \
          -target_partitions \
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
          -seeds \
         } flags {}

  # Tool
  set tools "chaco gpmetis mlpart"
  if { ![info exists keys(-tool)] } {
    puts "\[ERROR\] Missing mandatory argument -tool"
    return
  } elseif { !($keys(-tool) in $tools) } {
    puts "\[ERROR\] Invalid tool. Use one of the following: $tools"
    return
  } else {
     PartClusManager::set_tool $keys(-tool)
  }
       
  # Target partitions
  if { ![info exists keys(-target_partitions)] } {
    puts "\[ERROR\] Missing mandatory argument \"-target_partitions \[2, 32768\]\""
    return
  } elseif { !([string is integer $keys(-target_partitions)] && \
              $keys(-target_partitions) >= 2 && $keys(-target_partitions) <= 32768)} {
    puts "\[ERROR\] Argument -target_partitions should be an integer in the range \[2, 32768\]"
    return
  } else {
    PartClusManager::set_target_partitions $keys(-target_partitions) 
    if {$keys(-target_partitions) % 2} {
          PartClusManager::set_architecture "1 $keys(-target_partitions)"
    }
  }

  # Clique threshold
  if { [info exists keys(-clique_threshold)] } {
    if { !([string is integer $keys(-clique_threshold)] && \
           $keys(-clique_threshold) >= 3 && $keys(-clique_threshold) <= 32768) } {
      puts "\[ERROR\] Argument -clique_threshold should be an integer in the range \[3, 32768\]"
      return
    } else {
      PartClusManager::set_clique_threshold $keys(-clique_threshold)
    }
  }

  # Graph model
  set graph_models "clique star hybrid"
  if { [info exists keys(-graph_model)] } {
    if { $keys(-graph_model) in $graph_models } {
    } else {
      puts "\[ERROR\] Invalid graph model. Use one of the following: $graph_models"
      return
    }
    PartClusManager::set_graph_model $keys(-graph_model)
  }

  # Weight model
  if { [info exists keys(-weight_model)] } {
     if { !([string is integer $keys(-weight_model)] && \
             $keys(-weight_model) >= 1 && $keys(-weight_model) <= 7) } {
       puts "\[ERROR\] Argument -weight_model should be an integer in the range \[1, 7\]"
       return
     } else {
       PartClusManager::set_weight_model $keys(-weight_model)
     }     
  }

  # Max edge weight
  if { [info exists keys(-max_edge_weight)] } {
       if { !([string is integer $keys(-max_edge_weight)] && \
              $keys(-max_edge_weight) >= 1 && $keys(-max_edge_weight) <= 32768) } {
      puts "\[ERROR\] Argument -max_edge_weight should be an integer in the range \[1, 32768\]"
      return
    } else {
       PartClusManager::set_max_edge_weight $keys(-max_edge_weight)
    }       
  }

  # Max vertex weight
  if { [info exists keys(-max_vertex_weight)] } {
       if { !([string is integer $keys(-max_vertex_weight)] && \
              $keys(-max_vertex_weight) >= 1 && $keys(-max_vertex_weight) <= 32768) } {
      puts "\[ERROR\] Argument -max_vertex_weight should be an integer in the range \[1, 32768\]"
      return
    } else {
       PartClusManager::set_max_vertex_weight $keys(-max_vertex_weight)
    }       
  }

  # Num starts
  if { [info exists keys(-num_starts)] } {
       if { !([string is integer $keys(-num_starts)] && \
              $keys(-num_starts) >= 1 && $keys(-num_starts) <= 32768) } {
      puts "\[ERROR\] Argument -num_starts should be an integer in the range \[1, 32768\]"
      return
    } else {
       PartClusManager::set_num_starts $keys(-num_starts)
    }       
  }
  
  # Balance constraint
  if { [info exists keys(-balance_constraint)] } {
       if { !([string is integer $keys(-balance_constraint)] && \
              $keys(-balance_constraint) >= 0 && $keys(-balance_constraint) <= 50) } {
      puts "\[ERROR\] Argument -balance_constraint should be an integer in the range \[0, 50\]"
      return
    } else {
       PartClusManager::set_balance_constraint $keys(-balance_constraint)
    }       
  }

  # Coarsening ratio 
  if { [info exists keys(-coarsening_ratio)] } {
       if { !([string is double $keys(-coarsening_ratio)] && \
              $keys(-coarsening_ratio) >= 0.5 && $keys(-coarsening_ratio) <= 1.0) } {
      puts "\[ERROR\] Argument -coarsening_ratio should be a floating number in the range \[0.5, 1.0\]"
      return
    } else {
       PartClusManager::set_coarsening_ratio $keys(-coarsening_ratio)
    }       
  }

  # Coarsening vertices
  if { [info exists keys(-coarsening_vertices)] } {
       PartClusManager::set_coarsening_vertices $keys(-coarsening_vertices)
  }

  # Terminal propagation 
  if { [info exists keys(-enable_term_prop)] } {
       PartClusManager::set_enable_term_prop $keys(-enable_term_prop)
  }

  # Cut hop ratio 
  if { [info exists keys(-cut_hop_ratio)] } {
       if { !([string is double $keys(-cut_hop_ratio)] && \
              $keys(-cut_hop_ratio) >= 0.5 && $keys(-cut_hop_ratio) <= 1.0) } {
      puts "\[ERROR\] Argument -cut_hop_ratio should be a floating number in the range \[0.5, 1.0\]"
      return
    } else {
       PartClusManager::set_cut_hop_ratio $keys(-cut_hop_ratio)
    }       
  }

  # Architecture
  if { [info exists keys(-architecture)] } {
        PartClusManager::set_architecture $keys(-architecture)
  }

  # Seeds
  if { [info exists keys(-seeds)] } {
        PartClusManager::set_seeds $keys(-seeds)
  } else {
        if {! [info exists keys(-num_starts)]} {
              puts "\[ERROR\] Missing argument -seeds or -num_starts."
              return
        }
        PartClusManager::generate_seeds $keys(-num_starts)
  }
  set bestId [PartClusManager::run_partitioning]
  return $bestId
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
        PartClusManager::set_partition_ids_to_test $keys(-partition_ids)
  } else {
        puts "\[ERROR\] Missing argument -partition_ids."
        return
  }

  # Evaluation Function
  set functions "terminals hyperedges size area runtime hops"
  if { [info exists keys(-evaluation_function)] } {
        if { !($keys(-evaluation_function) in $functions) } {
          puts "\[ERROR\] Invalid function. Use one of the following: $functions"
          return
        }
        PartClusManager::set_evaluation_function $keys(-evaluation_function)
  }

  PartClusManager::evaluate_partitioning
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
    puts "\[ERROR\] Missing mandatory argument -partitioning_id"
    return
  } else {
    set partition_id $keys(-partitioning_id)
  } 
  
  PartClusManager::write_partitioning_to_db $partitioning_id

  if { [info exists keys(-dump_to_file)] } {
    PartClusManager::dump_part_id_to_file $keys(-dump_to_file)
  } 
}

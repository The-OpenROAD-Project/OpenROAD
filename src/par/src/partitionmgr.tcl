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
sta::define_cmd_args "triton_part_hypergraph" { -hypergraph_file hypergraph_file \
                                                [-fixed_file fixed_file] \
                                                [-num_parts num_parts] \
                                                [-balance_constraint balance_constraint] \
                                                [-vertex_dimension vertex_dimension] \
                                                [-hyperedge_dimension hyperedge_dimension] \
                                                [-seed seed] \
                                              }
proc triton_part_hypergraph { args } {
  sta::parse_key_args "triton_part_hypergraph" args \
      keys {-hypergraph_file
            -fixed_file
            -num_parts -balance_constraint
            -vertex_dimension
            -hyperedge_dimension
            -seed } \
      flags {}
 
  if { ![info exists keys(-hypergraph_file)] } {
    utl::error PAR 0924 "Missing mandatory argument -hypergraph_file."
  }
  set hypergraph_file $keys(-hypergraph_file)
  set fixed_file ""
  set num_parts 2
  set balance_constraint 1.0
  set seed 0
  set vertex_dimension 1
  set hyperedge_dimension 1
  
  if { [info exists keys(-fixed_file)] } {
    set fixed_file $keys(-fixed_file)
  }

  if { [info exists keys(-num_parts)] } {
    set num_parts $keys(-num_parts)
  }

  if { [info exists keys(-balance_constraint)] } {
    set balance_constraint $keys(-balance_constraint)
  }

  if { [info exists keys(-vertex_dimension)] } {
    set vertex_dimension $keys(-vertex_dimension)
  }

  if { [info exists keys(-hyperedge_dimension)] } {
    set hyperedge_dimension $keys(-hyperedge_dimension)
  }

  if { [info exists keys(-seed)] } {
    set seed $keys(-seed)
  }

  par::triton_part_hypergraph $hypergraph_file $fixed_file $num_parts \
      $balance_constraint $vertex_dimension $hyperedge_dimension $seed
}

sta::define_cmd_args "triton_part_design" { [-num_parts num_parts] \
                                            [-balance_constraint balance_constraint] \
                                            [-seed seed] \
                                            [-solution_file file_name] \
                                            [-paths_file file_name] \
                                            [-hypergraph_file file_name] \
                                          }
proc triton_part_design { args } {
  sta::parse_key_args "triton_part_design" args \
      keys {-num_parts
            -balance_constraint
            -seed
            -solution_file \
            -paths_file \
            -hypergraph_file } \
      flags {}
  set num_parts 2
  set balance_constraint 1.0
  set seed 0
  set hypergraph_file ""
  set paths_file ""
  set solution_file ""

  if { [info exists keys(-solution_file)] } {
      set solution_file $keys(-solution_file)
  }

  if { [info exists keys(-paths_file)] } {
      set paths_file $keys(-paths_file)
  }

  if { [info exists keys(-hypergraph_file)] } {
      set hypergraph_file $keys(-hypergraph_file)
  }

  if { [info exists keys(-num_parts)] } {
      set num_parts $keys(-num_parts)
  }

  if { [info exists keys(-balance_constraint)] } {
      set balance_constraint $keys(-balance_constraint)
  }

  if { [info exists keys(-seed)] } {
      set seed $keys(-seed)
  }

  par::triton_part_design $num_parts $balance_constraint $seed \
      $solution_file $paths_file $hypergraph_file
}

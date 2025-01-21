###############################################################################
##
## BSD 3-Clause License
##
# Copyright (c) 2020, The Regents of the University of California
# Copyright (c) 2024, Dimitris Fotakis
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
###############################################################################

sta::define_cmd_args "write_rcx_model" {
  [-file out_file_name]
}
proc write_rcx_model { args } {
  sta::parse_key_args "write_rcx_model" args keys { -file } flags { }

  set filename "rcx.model"
  if { [info exists keys(-file)] } {
    set filename $keys(-file)
  }
  rcx::write_rcx_model $filename
}
sta::define_cmd_args "init_rcx_model" {
  [-corner_names name_list]
  [-met_cnt met_cnt]
}
proc init_rcx_model { args } {
  sta::parse_key_args "init_rcx_model" args keys { -corner_names -met_cnt } flags { }

  set metal_cnt 0
  if { [info exists keys(-met_cnt)] } {
    set metal_cnt $keys(-met_cnt)
  }

  set corner_names ""
  if { [info exists keys(-corner_names)] } {
    set corner_names $keys(-corner_names)
  }
  rcx::init_rcx_model $corner_names $metal_cnt
}
sta::define_cmd_args "read_rcx_tables" {
  [-corner_name corner_name]
  [-file in_file_name]
  [-wire_index wire]
  [-over]
  [-under]
  [-over_under]
  [-diag]
}
proc read_rcx_tables { args } {
  sta::parse_key_args "read_rcx_tables" args keys { -corner_name -file -wire_index } \
    flags { -over -under -over_under -diag }

  set filename ""
  if { [info exists keys(-file)] } {
    set filename $keys(-file)
  }
  set corner ""
  if { [info exists keys(-corner_name)] } {
    set corner $keys(-corner_name)
  }
  set wire 0
  if { [info exists keys(-wire_index)] } {
    set wire $keys(-wire_index)
  }
  set over [info exists flags(-over)]
  set under [info exists flags(-under)]
  set over_under [info exists flags(-over_under)]
  set diag [info exists flags(-diag)]

  rcx::read_rcx_tables $corner $filename $wire $over $under $over_under $diag
}

sta::define_cmd_args "gen_solver_patterns" {
    [-process_file process_file]
    [-process_name process_name]
    [-version version]
    [-wire_cnt wire_count]
    [-len wire_len]
    [-w_list widths]
    [-s_list spacings]
    [-over_dist dist]
    [-under_dist dist]
}

proc gen_solver_patterns { args } {
  sta::parse_key_args "gen_solver_patterns" args \
    keys {-process_file -process_name -version -wire_cnt -len -w_list \
        -s_list -over_dist -under_dist } flags { }

  set process_file "MINTYPMAX"
  if { [info exists keys(-process_file)] } {
    set process_file $keys(-process_file)
  }
  set process_name ""
  if { [info exists keys(-process_name)] } {
    set process_name $keys(-process_name)
  }
  set version 1
  if { [info exists keys(-version)] } {
    set version $keys(-version)
  }
  set cnt 3
  if { [info exists keys(-wire_cnt)] } {
    set cnt $keys(-wire_cnt)
  }

  set len 10
  if { [info exists keys(-len)] } {
    set len $keys(-len)
  }
  set w_list "1"
  if { [info exists keys(-w_list)] } {
    set w_list $keys(-w_list)
  }
  set s_list "1.0 1.5 2.0 3 5"
  if { [info exists keys(-s_list)] } {
    set s_list $keys(-s_list)
  }
  set over_dist 4
  if { [info exists keys(-over_dist)] } {
    set over_dist $keys(-over_dist)
  }
  set under_dist 4
  if { [info exists keys(-under_dist)] } {
    set under_dist $keys(-under_dist)
  }

  rcx::gen_solver_patterns $process_file $process_name $version $cnt $len \
    $over_dist $under_dist $w_list $s_list
}

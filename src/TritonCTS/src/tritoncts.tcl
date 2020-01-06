#////////////////////////////////////////////////////////////////////////////////////
#// Authors: Mateus Fogaca
#//          (Ph.D. advisor: Ricardo Reis)
#//          Jiajia Li
#//          Andrew Kahng
#// Based on:
#//          K. Han, A. B. Kahng and J. Li, "Optimal Generalized H-Tree Topology and 
#//          Buffering for High-Performance and Low-Power Clock Distribution", 
#//          IEEE Trans. on CAD (2018), doi:10.1109/TCAD.2018.2889756.
#//
#//
#// BSD 3-Clause License
#//
#// Copyright (c) 2018, The Regents of the University of California
#// All rights reserved.
#//
#// Redistribution and use in source and binary forms, with or without
#// modification, are permitted provided that the following conditions are met:
#//
#// * Redistributions of source code must retain the above copyright notice, this
#//   list of conditions and the following disclaimer.
#//
#// * Redistributions in binary form must reproduce the above copyright notice,
#//   this list of conditions and the following disclaimer in the documentation
#//   and/or other materials provided with the distribution.
#//
#// * Neither the name of the copyright holder nor the names of its
#//   contributors may be used to endorse or promote products derived from
#//   this software without specific prior written permission.
#//
#// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
#// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
#// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
#// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
#// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
#// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
#// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
#// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
#// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#////////////////////////////////////////////////////////////////////////////////////

sta::define_cmd_args "clock_tree_synthesis" {[-lut_file lut] \
                                             [-sol_list slist] \
                                             [-root_buf buf] \
                                             [-wire_unit unit] \
                                             [-clk_nets nets] \ 
                                            } 

proc clock_tree_synthesis { args } {
  sta::parse_key_args "clock_tree_synthesis" args \
    keys {-lut_file -sol_list -root_buf -wire_unit -clk_nets} flags {}

  set cts [get_triton_cts]

  if { [info exists keys(-lut_file)] } {
        set lut $keys(-lut_file)
  } else {
        puts "Missing argument -lut_file"
        exit
  }
 
  if { [info exists keys(-sol_list)] } {
        set sol_list $keys(-sol_list)
  } else {
        puts "Missing argument -sol_list"
        exit
  }

  if { [info exists keys(-root_buf)] } {
        set root_buf $keys(-root_buf)
  } else {
        puts "Missing argument -root_buf"
        exit
  }

  if { [info exists keys(-wire_unit)] } {
        set wire_unit $keys(-wire_unit)
  } else {
        puts "Missing argument -wire_unit"
        exit
  }

  if { [info exists keys(-clk_nets)] } {
        set clk_nets $keys(-clk_nets)
        $cts set_clock_nets $clk_nets
  }

  $cts set_lut_file $lut 
  $cts set_sol_list_file $sol_list
  $cts set_wire_segment_distance_unit $wire_unit
  $cts set_root_buffer $root_buf
  $cts run_triton_cts
}

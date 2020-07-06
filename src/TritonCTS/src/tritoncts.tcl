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
                                             [-buf_list buflist] \
                                             [-root_buf buf] \
                                             [-wire_unit unit] \
                                             [-max_cap cap] \
                                             [-max_slew slew] \
                                             [-clk_nets nets] \ 
                                             [-out_path path] \
                                             [-sqr_cap capvalue] \
                                             [-sqr_res resvalue] \
                                             [-slew_inter slewvalue] \
                                             [-cap_inter capvalue] \
                                             [-characterization_only] \
                                            } 

proc clock_tree_synthesis { args } {
  sta::parse_key_args "clock_tree_synthesis" args \
    keys {-lut_file -sol_list -root_buf -buf_list -wire_unit -max_cap -max_slew -clk_nets -out_path -sqr_cap -sqr_res -slew_inter -cap_inter} flags {-characterization_only}

  set cts [get_triton_cts]

  #Clock Tree Synthesis TCL -> Required commands:
  #                               -lut_file , -sol_list , -root-buf, -wire_unit
  #                                               or
  #                               -buf_list , -sqr_cap , -sqr_res
  #                         -> Other commands can be used as extra parameters or in conjunction with each other:
  #                               ex: clock_tree_synthesis -buf_list "BUFX1 BUFX2" -wire_unit 20 -sqr_cap 1 -sqr_res 2 -clk_nets clk1


  $cts set_only_characterization [info exists flags(-characterization_only)]

  if { [info exists keys(-lut_file)] } {
	  set lut $keys(-lut_file)
    $cts set_lut_file $lut 
  } 
 
  if { [info exists keys(-sol_list)] } {
	  set sol_list $keys(-sol_list)
    $cts set_sol_list_file $sol_list
  } 

  if { [info exists keys(-buf_list)] } {
    set buf_list $keys(-buf_list)
    $cts set_buffer_list $buf_list
  } else {
    if {![info exists keys(-lut_file)] || ![info exists keys(-sol_list)]} {
      #User must either input a lut file or the buffer list.
      ord::error "Missing argument -buf_list or -lut_file / -sol_list"
    }
  }

  if { [info exists keys(-wire_unit)] } {
    set wire_unit $keys(-wire_unit)
    $cts set_wire_segment_distance_unit $wire_unit
  } 

  if { [info exists keys(-max_cap)] } {
    set max_cap_value $keys(-max_cap)
    $cts set_max_char_cap $max_cap_value
  } 

  if { [info exists keys(-max_slew)] } {
    set max_slew_value $keys(-max_slew)
    $cts set_max_char_slew $max_slew_value
  } 

  if { [info exists keys(-clk_nets)] } {
    set clk_nets $keys(-clk_nets)
    set fail [$cts set_clock_nets $clk_nets]
    if {$fail} {
      ord::error "Error when finding -clk_nets in DB!"
    }
  }

  if { [info exists keys(-slew_inter)] } {
	  set slew $keys(-slew_inter)
    $cts set_slew_inter $slew 
  } 

  if { [info exists keys(-cap_inter)] } {
	  set cap $keys(-cap_inter)
    $cts set_cap_inter $cap 
  } 

  if { [info exists keys(-root_buf)] } {
    set root_buf $keys(-root_buf)
    $cts set_root_buffer $root_buf
  } else {
    if { [info exists keys(-buf_list)] } {
      #If using -buf_list, the first buffer can become the root buffer.
      $cts set_root_buffer [lindex $buf_list 0]
    } else {
      #User must enter at least one of -root_buf or -buf_list.
      ord::error "Missing argument -root_buf"
    }
  }

  if { [info exists keys(-out_path)] && (![info exists keys(-lut_file)] || ![info exists keys(-sol_list)]) } {
    set out_path $keys(-out_path)
    $cts set_out_path $out_path
  }

  if {![info exists keys(-lut_file)] || ![info exists keys(-sol_list)]} {
    if { [info exists keys(-sqr_cap)] && [info exists keys(-sqr_res)] } {
      set sqr_cap $keys(-sqr_cap)
      $cts set_cap_per_sqr $sqr_cap
      set sqr_res $keys(-sqr_res)
      $cts set_res_per_sqr $sqr_res
    } else {
      #User must enter capacitance and resistance per square (um²) when creating a new characterization.
      ord::error "Missing argument -sqr_cap and/or -sqr_res"
    }
  }

  if {[catch {$cts run_triton_cts} error_msg options]} {
    puts $error_msg
  }

  # CTS changed the network behind the STA's back.
  sta::network_changed
}

sta::define_cmd_args "report_cts" {[-out_file file] \
                                  } 

proc report_cts { args } {
  sta::parse_key_args "report_cts" args \
    keys {-out_file} flags {}

  set cts [get_triton_cts]

  if { [info exists keys(-out_file)] } {
	  set outFile $keys(-out_file)
    $cts set_metric_output $outFile 
  } 

  $cts report_cts_metrics
}

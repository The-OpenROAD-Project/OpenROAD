###############################################################################
##
## BSD 3-Clause License
##
# Copyright (c) 2020, The Regents of the University of California
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

sta::define_cmd_args "define_process_corner" {
    [-ext_model_index index] filename
}

proc define_process_corner { args } {
  sta::parse_key_args "define_process_corner" args \
    keys {-ext_model_index} flags {}
  sta::check_argc_eq1 "define_process_corner" $args

  set ext_model_index 0
  if { [info exists keys(-ext_model_index)] } {
    set ext_model_index $keys(-ext_model_index)
  }

  set filename [file nativename [lindex $args 0]]

  rcx::define_process_corner $ext_model_index $filename
}

sta::define_cmd_args "define_rcx_corners" {
    [-corner_list cornerList]
}

proc define_rcx_corners { args } {
  sta::parse_key_args "define_rcx_corners" args \
    keys { -corner_list } flags {}

  set corner_list ""
  if { [info exists keys(-corner_list)] } {
    set corner_list $keys(-corner_list)
  }
  rcx::define_rcx_corners $corner_list
}

sta::define_cmd_args "get_model_corners" {
    [-ext_model_file file_name]
}

proc get_model_corners { args } {
  sta::parse_key_args "get_model_corners" args \
    keys { -ext_model_file } flags {}

  set ext_model_file ""
  if { [info exists keys(-ext_model_file)] } {
    set ext_model_file $keys(-ext_model_file)
  }
  rcx::get_model_corners $ext_model_file
}

sta::define_cmd_args "extract_parasitics" {
    [-ext_model_file filename]
    [-corner cornerIndex]
    [-corner_cnt count]
    [-max_res ohms]
    [-coupling_threshold fF]
    [-debug_net_id id]
    [-dbg dbg_num ]
    [-lef_res]
    [-lef_rc]
    [-cc_model track]
    [-context_depth depth]
    [-no_merge_via_res]
    [-skip_over_cell ]
    [-version]
}
proc extract_parasitics { args } {
  sta::parse_key_args "extract_parasitics" args \
    keys { -ext_model_file
           -corner
           -corner_cnt
           -max_res
           -coupling_threshold
           -debug_net_id
           -dbg
           -cc_model
           -context_depth
           -version } \
    flags { -lef_res -lef_rc
            -no_merge_via_res -skip_over_cell }

  set ext_model_file ""
  if { [info exists keys(-ext_model_file)] } {
    set ext_model_file $keys(-ext_model_file)
  }

  set corner_cnt 1
  if { [info exists keys(-corner_cnt)] } {
    set corner_cnt $keys(-corner_cnt)
    sta::check_positive_integer "-corner_cnt" $corner_cnt
  }
  set corner -1
  if { [info exists keys(-corner)] } {
    set corner $keys(-corner)
    sta::check_positive_integer "-corner" $corner
  }

  set max_res 50.0
  if { [info exists keys(-max_res)] } {
    set max_res $keys(-max_res)
    sta::check_positive_float "-max_res" $max_res
  }

  set coupling_threshold 0.1
  if { [info exists keys(-coupling_threshold)] } {
    set coupling_threshold $keys(-coupling_threshold)
    sta::check_positive_float "-coupling_threshold" $coupling_threshold
  }

  set lef_rc [info exists flags(-lef_rc)]
  set lef_res [info exists flags(-lef_res)]
  set no_merge_via_res [info exists flags(-no_merge_via_res)]
  set skip_over_cell [info exists flags(-skip_over_cell)]

  set cc_model 10
  if { [info exists keys(-cc_model)] } {
    set cc_model $keys(-cc_model)
  }

  set depth 5
  if { [info exists keys(-context_depth)] } {
    set depth $keys(-context_depth)
    sta::check_positive_integer "-context_depth" $depth
  }

  set debug_net_id ""
  if { [info exists keys(-debug_net_id)] } {
    set debug_net_id $keys(-debug_net_id)
  }
  set dbg 0
  if { [info exists keys(-dbg)] } {
    set dbg $keys(-dbg)
  }
  set version 1.0
  if { [info exists keys(-version)] } {
    set version $keys(-version)
    sta::check_positive_float "-version" $version
  }
  rcx::extract $ext_model_file $corner_cnt $max_res \
    $coupling_threshold $cc_model \
    $depth $debug_net_id $lef_res $no_merge_via_res \
    $lef_rc $skip_over_cell $version $corner $dbg
}

sta::define_cmd_args "write_spef" {
  [-net_id net_id]
  [-nets nets]
  [-coordinates]
  filename }

proc write_spef { args } {
  sta::parse_key_args "write_spef" args \
    keys { -net_id -nets } \
    flags { -coordinates }
  sta::check_argc_eq1 "write_spef" $args

  set spef_file $args

  set nets ""
  if { [info exists keys(-nets)] } {
    set nets $keys(-nets)
  }

  set net_id 0
  if { [info exists keys(-net_id)] } {
    set net_id $keys(-net_id)
  }

  set coordinates [info exists flags(-coordinates)]

  rcx::write_spef $spef_file $nets $net_id $coordinates
}

sta::define_cmd_args "adjust_rc" {
    [-res_factor res]
    [-cc_factor cc]
    [-gndc_factor gndc]
}

proc adjust_rc { args } {
  sta::parse_key_args "adjust_rc" args \
    keys { -res_factor
           -cc_factor
           -gndc_factor } \
    flags {}

  set res_factor 1.0
  if { [info exists keys(-res_factor)] } {
    set res_factor $keys(-res_factor)
    sta::check_positive_float "-res_factor" $res_factor
  }

  set cc_factor 1.0
  if { [info exists keys(-cc_factor)] } {
    set cc_factor $keys(-cc_factor)
    sta::check_positive_float "-cc_factor" $cc_factor
  }

  set gndc_factor 1.0
  if { [info exists keys(-gndc_factor)] } {
    set gndc_factor $keys(-gndc_factor)
    sta::check_positive_float "-gndc_factor" $gndc_factor
  }

  rcx::adjust_rc $res_factor $cc_factor $gndc_factor
}
sta::define_cmd_args "diff_spef" {
    [-file filename]
    [-spef_corner spef_num]
    [-ext_corner ext_num]
    [-r_res]
    [-r_cap]
    [-r_cc_cap]
    [-r_conn]
}
proc diff_spef { args } {
  sta::parse_key_args "diff_spef" args \
    keys { -file -spef_corner -ext_corner } \
    flags { -r_res -r_cap -r_cc_cap -r_conn }

  set filename ""
  if { [info exists keys(-file)] } {
    set filename [file nativename $keys(-file)]
  }
  set res [info exists flags(-over)]
  set cap [info exists flags(-over)]
  set cc_cap [info exists flags(-over)]
  set conn [info exists flags(-over)]

  set spef_corner -1
  if { [info exists keys(-spef_corner)] } {
    set spef_corner $keys(-spef_corner)
  }
  set ext_corner -1
  if { [info exists keys(-ext_corner)] } {
    set ext_corner $keys(-ext_corner)
  }
  rcx::diff_spef $filename $conn $res $cap $cc_cap $spef_corner $ext_corner
}
sta::define_cmd_args "bench_wires" {
    [-met_cnt mcnt]
    [-cnt count]
    [-len wire_len]
    [-over]
    [-diag]
    [-all]
    [-db_only]
    [-v1]
    [-under_met layer]
    [-w_list width]
    [-s_list space]
    [-over_dist dist]
    [-under_dist dist]
}

proc bench_wires { args } {
  sta::parse_key_args "bench_wires" args \
    keys { -met_cnt -cnt -len -under_met
           -w_list -s_list -over_dist -under_dist } \
    flags { -diag -over -all -db_only -v1 }

  if { ![ord::db_has_tech] } {
    utl::error RCX 357 "No LEF technology has been read."
  }

  set over [info exists flags(-over)]
  set all [info exists flags(-all)]
  set diag [info exists flags(-diag)]
  set db_only [info exists flags(-db_only)]
  set v1 [info exists flags(-v1)]

  set met_cnt 1000
  if { [info exists keys(-met_cnt)] } {
    set met_cnt $keys(-met_cnt)
  }

  set cnt 5
  if { [info exists keys(-cnt)] } {
    set cnt $keys(-cnt)
  }

  set len 100
  if { [info exists keys(-len)] } {
    set len $keys(-len)
  }

  set under_met -1
  if { [info exists keys(-under_met)] } {
    set under_met $keys(-under_met)
  }

  set w_list "1"
  if { [info exists keys(-w_list)] } {
    set w_list $keys(-w_list)
  }

  set s_list "1 2 2.5 3 3.5 4 4.5 5 6 8 10 12"
  if { [info exists keys(-s_list)] } {
    set s_list $keys(-s_list)
  }

  set over_dist 100
  if { [info exists keys(-over_dist)] } {
    set over_dist $keys(-over_dist)
  }

  set under_dist 100
  if { [info exists keys(-under_dist)] } {
    set under_dist $keys(-under_dist)
  }

  rcx::bench_wires $db_only $over $diag $all $met_cnt $cnt $len \
    $under_met $w_list $s_list $over_dist $under_dist $v1
}

sta::define_cmd_args "bench_verilog" { filename }

proc bench_verilog { args } {
  sta::parse_key_args "bench_verilog" args keys {} flags{}
  sta::check_argc_eq1 "bench_verilog" $args
  rcx::bench_verilog $args
}

sta::define_cmd_args "bench_read_spef" { filename }

proc bench_read_spef { args } {
  sta::parse_key_args "bench_read_spef" args keys {} flags{}
  sta::check_argc_eq1 "bench_read_spef" $args
  rcx::read_spef $args
}

sta::define_cmd_args "gen_rcx_model" {
    [-spef_file_list spefList]
    [-corner_list cornerList]
    [-out_file outfilename]
    [-comment comment]
    [-version version]
    [-pattern pattern]
}

proc gen_rcx_model { args } {
  sta::parse_key_args "gen_rcx_model" args \
    keys { -spef_file_list -corner_list -out_file -comment -version -pattern } \
    flags {}

  set spef_file_list ""
  if { [info exists keys(-spef_file_list)] } {
    set spef_file_list $keys(-spef_file_list)
  }
  set corner_list ""
  if { [info exists keys(-corner_list)] } {
    set corner_list $keys(-corner_list)
  }
  set out_file "rcx.model"
  if { [info exists keys(-out_file)] } {
    set out_file $keys(-out_file)
  }
  set version "2.0"
  if { [info exists keys(-version)] } {
    set version $keys(-version)
  }
  set comment "RCX Model File"
  if { [info exists keys(-comment)] } {
    set comment $keys(-comment)
  }
  set pattern 0
  if { [info exists keys(-pattern)] } {
    set pattern $keys(-pattern)
  }

  rcx::gen_rcx_model $spef_file_list $corner_list $out_file $comment $version $pattern
}

sta::define_cmd_args "write_rules" {
    [-file filename]
    [-dir dir]
    [-name name]
    [-db]
}

proc write_rules { args } {
  sta::parse_key_args "write_rules" args \
    keys { -file -dir -name } \
    flags { -db }

  set filename "extRules"
  if { [info exists keys(-file)] } {
    set filename $keys(-file)
  }

  set dir "./"
  if { [info exists keys(-dir)] } {
    set dir $keys(-dir)
  }

  set name "TYP"
  if { [info exists keys(-name)] } {
    set name $keys(-name)
  }
  rcx::write_rules $filename $dir $name
}
sta::define_cmd_args "bench_wires_gen" {
    [  -len		length_in_min_widths ]
    [	 -met	  	metal	 ]
    [	 -mlist	  	metal_list	 ]
    [	 -width	  	multiplier_width_list	 ]
    [	 -spacing	  	multiplier_spacing_list	 ]
    [	 -couple_width	  	multiplier_coupling_width_list	 ]
    [	 -couple_spacing	  	multiplier_coupling_spacing_list ]
    [	 -over_width	  	multiplier_over_width_list	 ]
    [	 -over_spacing	  	multiplier_over_spacing_list	 ]
    [	 -under_width	  	multiplier_under_width_list	 ]
    [	 -under_spacing	  	multiplier_under_spacing_list	 ]
    [	 -over2_width	  	multiplier_over2_width_list	 ]
    [	 -over2_spacing	  	multiplier_over2_spacing_list	 ]
    [	 -under2_width	  	multiplier_under2_width_list	 ]
    [	 -under2_spacing	  	multiplier_under2_spacing_list	 ]
    [	 -dbg	  	dbg_flag	 ]
    [	 -wire_cnt	  	wire_count	 ]
    [	 -offset_over	  	offset_over	 ]
    [	 -offset_under	  	offset_under	 ]
    [	 -under_dist	  	max_dist_to_under_met	 ]
    [	 -over_dist	  	max_dist_to_over_met	 ]
    [  -diag ]
    [  -over ]
    [  -under ]
    [  -over_under ]
}
proc get_arg_val { keys name default_value } {
  puts " $default_value $name $keys($name) "
  set v $default_value
  if { [info exists keys($name)] } {
    set v $keys($name)
  }
  return $v
}
proc bench_wires_gen { args } {
  sta::parse_key_args "bench_wires_gen" args keys \
    { -len -width -spacing -couple_width -couple_spacing -over_width \
        -over_spacing -under_width -under_spacing -over2_width -over2_spacing \
        -under2_width -under2_spacing -dbg -wire_cnt -mlist -offset_over \
        -offset_under -under_dist -over_dist -met } \
    flags { -diag -over -under -over_under }

  set width "1, 1.5, 2"
  set spacing "1, 1.5, 2, 3, 4, 6, 8, 10"
  set couple_width "1"
  set couple_spacing "1"
  set over_width "1, 2"
  set over_spacing "1, 2"
  set under_width "1, 2"
  set under_spacing "1, 2"
  set over2_width "1"
  set over2_spacing "1"
  set under2_width "1"
  set under2_spacing "1"
  set dbg 1
  set wire_cnt 5
  set mlist ALL
  set len 10
  set offset_over "0.1"
  set offset_under "0.1"
  set under_dist 1000
  set over_dist 1000
  set met 0
  set over [info exists flags(-over)]
  set under [info exists flags(-under)]
  set over_under [info exists flags(-over_under)]
  set diag [info exists flags(-diag)]

  if { [info exists keys(-width)] } { set width $keys(-width) }
  if { [info exists keys(-spacing)] } { set spacing $keys(-spacing) }
  if { [info exists keys(-couple_width)] } { set couple_width $keys(-couple_width) }
  if { [info exists keys(-couple_spacing)] } { set couple_spacing $keys(-couple_spacing) }
  if { [info exists keys(-over_width)] } { set over_width $keys(-over_width) }
  if { [info exists keys(-over_spacing)] } { set over_spacing $keys(-over_spacing) }
  if { [info exists keys(-under_width)] } { set under_width $keys(-under_width) }
  if { [info exists keys(-under_spacing)] } { set under_spacing $keys(-under_spacing) }
  if { [info exists keys(-over2_width)] } { set over2_width $keys(-over2_width) }
  if { [info exists keys(-over2_spacing)] } { set over2_spacing $keys(-over2_spacing) }
  if { [info exists keys(-under2_width)] } { set under2_width $keys(-under2_width) }
  if { [info exists keys(-under2_spacing)] } { set under2_spacing $keys(-under2_spacing) }
  if { [info exists keys(-dbg)] } { set dbg $keys(-dbg) }
  if { [info exists keys(-wire_cnt)] } { set wire_cnt $keys(-wire_cnt) }
  if { [info exists keys(-mlist)] } { set mlist $keys(-mlist) }
  if { [info exists keys(-len)] } { set len $keys(-len) }

  if { [info exists keys(-offset_over)] } { set offset_over $keys(-offset_over) }
  if { [info exists keys(-offset_under)] } { set offset_under $keys(-offset_under) }
  if { [info exists keys(-under_dist)] } { set under_dist $keys(-under_dist) }
  if { [info exists keys(-over_dist)] } { set over_dist $keys(-over_dist) }
  if { [info exists keys(-met)] } { set met $keys(-met) }


  if { $dbg > 0 } {
    puts "width = $width"
    puts "spacing = $spacing"
    puts "couple_width = $couple_width"
    puts "couple_spacing = $couple_spacing"
    puts "over_width = $over_width"
    puts "over_spacing = $over_spacing"
    puts "under_width = $under_width"
    puts "under_spacing = $under_spacing"
    puts "over2_width = $over2_width"
    puts "over2_spacing = $over2_spacing"
    puts "under2_width = $under2_width"
    puts "under2_spacing = $under2_spacing"
    puts "dbg = $dbg"
    puts "wire_cnt = $wire_cnt"
    puts "mlist = $mlist"
    puts "offset_over = $offset_over"
    puts "offset_under = $offset_under"
    puts "under_dist = $under_dist"
    puts "over_dist = $over_dist"
    puts "over = $over"
    puts "under = $under"
    puts "over_under = $over_under"
    puts "met = $met"
  }
  rcx::bench_wires_gen $width $spacing $couple_width $couple_spacing \
    $over_width $over_spacing $under_width $under_spacing $over2_width \
    $over2_spacing $under2_width $under2_spacing $dbg $wire_cnt $mlist \
    $len $offset_over $offset_under $under_dist $over_dist $over $under \
    $over_under $met
}

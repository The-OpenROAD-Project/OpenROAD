# Copyright (c) 2020, OpenROAD
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

sta::define_cmd_args "define_process_corner" {
    [-ext_model_index index] filename
}

proc define_process_corner { args } {
  sta::parse_key_args "define_process_corner" args keys {-ext_model_index}
  sta::check_argc_eq1 "define_process_corner" $args

  set ext_model_index 0
  if { [info exists keys(-ext_model_index)] } {
    set ext_model_index $keys(-ext_model_index)
  }

  set filename [file nativename [lindex $args 0]]

  rcx::define_process_corner $ext_model_index $filename
}

sta::define_cmd_args "extract_parasitics" {
    [-ext_model_file filename]
    [-corner_cnt count]
    [-max_res ohms]
    [-coupling_threshold fF]
    [-signal_table value]
    [-debug_net_id id]
    [-lef_res]
    [-cc_model track]
    [-context_depth depth]
    [-no_merge_via_res]
}

proc extract_parasitics { args } {
  sta::parse_key_args "extract_parasitics" args keys \
      { -ext_model_file
        -corner_cnt
        -max_res
        -coupling_threshold
        -signal_table
        -debug_net_id
        -context_depth
        -cc_model } \
      flags { -lef_res }

  set ext_model_file ""
  if { [info exists keys(-ext_model_file)] } {
    set ext_model_file $keys(-ext_model_file)
  }

  set corner_cnt 1
  if { [info exists keys(-corner_cnt)] } {
    set corner_cnt $keys(-corner_cnt)
    sta::check_positive_integer "-corner_cnt" $corner_cnt
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

  set signal_table 3
  if { [info exists keys(-signal_table)] } {
    set signal_table $keys(-signal_table)
    sta::check_positive_integer "-signal_table" $signal_table
  }

  set lef_res [info exists flags(-lef_res)]
  set no_merge_via_res [info exists flags(-no_merge_via_res)]

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

  rcx::extract $ext_model_file $corner_cnt $max_res \
      $coupling_threshold $signal_table $cc_model \
      $depth $debug_net_id $lef_res $no_merge_via_res
}

sta::define_cmd_args "write_spef" { 
  [-net_id net_id]
  [-nets nets] filename }

proc write_spef { args } {
  sta::parse_key_args "write_spef" args keys \
      { -net_id 
        -nets }
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

  rcx::write_spef $spef_file $nets $net_id
}

sta::define_cmd_args "adjust_rc" {
    [-res_factor res]
    [-cc_factor cc]
    [-gndc_factor gndc]
}

proc adjust_rc { args } {
  sta::parse_key_args "adjust_rc" args keys \
      { -res_factor
        -cc_factor
        -gndc_factor }

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
    [-r_res]
    [-r_cap]
    [-r_cc_cap]
    [-r_conn]
}

proc diff_spef { args } {
  sta::parse_key_args "diff_spef" args keys \
      { -file } \
      flags { -r_res -r_cap -r_cc_cap -r_conn }
  
  set filename "" 
  if { [info exists keys(-file)] } {
    set filename [file nativename $keys(-file)]
  }
  set res [info exists flags(-over)]
  set cap [info exists flags(-over)]
  set cc_cap [info exists flags(-over)]
  set conn [info exists flags(-over)]

  rcx::diff_spef $filename $conn $res $cap $cc_cap
}

sta::define_cmd_args "bench_wires" {
    [-met_cnt mcnt]
    [-cnt count]
    [-len wire_len]
    [-over]
    [-diag]
    [-all]
    [-db_only]
    [-under_met layer]
    [-w_list width]
    [-s_list space]
}

proc bench_wires { args } {
  sta::parse_key_args "bench_wires" args keys \
      { -met_cnt -cnt -len -under_met
        -w_list -s_list } \
      flags { -diag -over -all -db_only }

  if { ![ord::db_has_tech] } {
    utl::error RCX 357 "No LEF technology has been read."
  }

  set over [info exists flags(-over)]
  set all [info exists flags(-all)]
  set diag [info exists flags(-diag)]
  set db_only [info exists flags(-db_only)]

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
  
  rcx::bench_wires $db_only $over $diag $all $met_cnt $cnt $len $under_met $w_list $s_list 
}

sta::define_cmd_args "bench_verilog" { filename }

proc bench_verilog { args } {
  sta::check_argc_eq1 "bench_verilog" $args
  rcx::bench_verilog $args
}

sta::define_cmd_args "bench_read_spef" { filename }

proc bench_read_spef { args } {
  sta::check_argc_eq1 "bench_read_spef" $args
  rcx::read_spef $args
}

sta::define_cmd_args "write_rules" {
    [-file filename]
    [-dir dir]
    [-name name]
    [-pattern pattern]
    [-read_from_solver]
    [-db]
}

proc write_rules { args } {
  sta::parse_key_args "write_rules" args keys \
      { -file -dir -name -pattern } \
      flags { -read_from_solver -db }
  
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

  set pattern 0
  if { [info exists keys(-pattern)] } {
    set pattern $keys(-pattern)
  }
  set solver [info exists flags(-read_from_solver)]
  set db [info exists flags(-db)]

 rcx::write_rules $filename $dir $name $pattern $db $solver
}


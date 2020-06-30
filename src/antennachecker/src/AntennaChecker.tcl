# BSD 3-Clause License
#
# Copyright (c) 2020, MICL, DD-Lab, University of Michigan
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.



sta::define_cmd_args "check_antennas" { [-verbose] }
sta::define_cmd_args "get_met_rest_length" { [-net_name netname]\
                                              [-route_level rt_lv] }
sta::define_cmd_args "check_net_violation" { [-net_name netname] }

proc load_antenna_rules { } {
  antenna_checker::load_antenna_rules
}


proc check_antennas { args } {
  sta::parse_key_args "check_antennas" args \
  keys {} \
  flags {-verbose}

  antenna_checker::antennachecker_set_verbose [info exists flags(-verbose)]
  antenna_checker::check_antennas
}

proc get_met_avail_length { args } {
  sta::parse_key_args "get_met_rest_length" args \
    keys {-net_name -route_level} \
    flags {}

  if { [info exists keys(-net_name)] } {
    set netname $keys(-net_name)
    antenna_checker::antennachecker_set_net_name $netname

    if { [info exists keys(-route_level)] } {
      set rt_lv $keys(-route_level)
      sta::check_positive_integer "-route_level" $rt_lv
      antenna_checker::antennachecker_set_route_level $rt_lv
    } else {
      ord::error "no -route_level specified."
    }
  } else {
    ord::error "no -net_name specified."
  }
  antenna_checker::get_met_avail_length
}

proc check_net_violation { args } {
  sta::parse_key_args "check_net_violation" args \
  keys {-net_name} \
  flags {}

  if { [info exists keys(-net_name)] } {
    set netname $keys(-net_name)
    set res [antenna_checker::check_net_violation $netname]
    
    return $res
  } else {
    ord::error "no -net_name specified."
  }  
  
  return 0
}


proc add_antenna_cell { net antenna_cell_name sink_inst antenna_inst_name } {

  set block [[[::ord::get_db] getChip] getBlock]
  set net_name [$net getName]

  set antenna_master [[::ord::get_db] findMaster $antenna_cell_name]
  set antenna_mterm [$antenna_master getMTerms]

  set inst_loc_x [lindex [$sink_inst getLocation] 0]
  set inst_loc_y [lindex [$sink_inst getLocation] 1]
  set inst_ori [$sink_inst getOrient]

  set antenna_inst [odb::dbInst_create $block $antenna_master $antenna_inst_name]
  set antenna_iterm [$antenna_inst findITerm "A"]

  $antenna_inst setLocation $inst_loc_x $inst_loc_y
  $antenna_inst setOrient $inst_ori
  $antenna_inst setPlacementStatus PLACED
  odb::dbITerm_connect $antenna_iterm $net

}


proc antenna_fixing {} {

  set block [[[::ord::get_db] getChip] getBlock]
  
  foreach inst [$block getInsts] {
    if {[[$inst getMaster] getType] == "CORE_SPACER"} {
      odb::dbInst_destroy $inst
    }
  }
  
  set antenna_cell_name "ANTENNA3_A9PP84TR_C14"
  
  set target_file "6_final_with_diodes"
  
  set iterate_times 0
  
  set antenna_node_counts 0
  
  while { $iterate_times < 1 } {
  
    set nets [$block getNets]
  
    foreach net $nets {
      set net_name [$net getConstName]
      set flag [check_net_violation -net_name $net_name]
      if {$flag == 0} {
        continue
      }
  
      if { [$net isSpecial] } {
        continue
      }
  
      foreach iterm [$net getITerms] {
        set inst [$iterm getInst]
  
        dict set inst_count $inst [expr [dict get $inst_count $inst] + 1]
  
        set count [dict get $inst_count $inst]
  
        set antenna_inst_name "ANTENNA"
        append antenna_inst_name "_" [$inst getName] "_" $count
  
        if {[catch {add_antenna_cell $net $antenna_cell_name $inst $antenna_inst_name} result] } {
          puts "adding node failed"
          continue
        } else {
          set antenna_inst [$block findInst $antenna_inst_name]
          dict set inst_count $antenna_inst 1
          set antenna_node_counts [expr $antenna_node_counts + 1]
        }
  
        break
  
      }
  
      if { $antenna_node_counts == 4 } {
        break
      }
    }
  
    set iterate_times [expr $iterate_times + 1]
  
  }
  
  set verilog_file_name "$target_file.v"
  write_verilog  $verilog_file_name
  
  set def_file_name "$target_file.def"
  write_def $def_file_name

}

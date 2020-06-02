#############################################################################
##
## BSD 3-Clause License
##
## Copyright (c) 2019, James Cherry, Parallax Software, Inc.
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
##   and/or other materials provided with the distribution.
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
#############################################################################

# -constraints is an undocumented option for worthless academic contests
sta::define_cmd_args "detailed_placement" {[-constraints constraints_file]}

proc detailed_placement { args } {
  sta::parse_key_args "detailed_placement" args \
    keys {-constraints} flags {}

  if { [info exists keys(-max_displacment)] } {
    set max_displacment $keys(-max_displacment)
    sta::check_positive_integer "-max_displacment" $max_displacment
  } else {
    set max_displacment 0
  }

  sta::check_argc_eq0 "detailed_placement" $args
  if { [ord::db_has_rows] } {
    opendp::detailed_placement_cmd $max_displacment
  } else {
    ord::error "no rows defined in design. Use initialize_floorplan to add rows."
  }
}

sta::define_cmd_args "set_placement_padding" { -global|-masters masters|-instances insts\
						 [-right site_count]\
						 [-left site_count] \
						 [instances]\
					       }

proc set_placement_padding { args } {
  sta::parse_key_args "set_placement_padding" args \
    keys {-masters -instances -right -left} flags {-global}

  set left 0
  if { [info exists keys(-left)] } {
    set left $keys(-left)
    sta::check_positive_integer "-left" $left
  }
  set right 0
  if { [info exists keys(-right)] } {
    set right $keys(-right)
    sta::check_positive_integer "-right" $right
  }

  sta::check_argc_eq0 "set_placement_padding" $args
  if { [info exists flags(-global)] } {
    opendp::set_padding_global $left $right
  } elseif { [info exists keys(-masters)] } {
    set masters [opendp::get_masters_arg "-masters" $keys(-masters)]
    foreach master $masters {
      opendp::set_padding_master $master $left $right
    }
  } elseif { [info exists keys(-instances)] } {
    # sta::get_instances_error supports sdc get_cells
    set insts [sta::get_instances_error "-instances" $keys(-instances)]
    foreach inst $insts {
      set db_inst [sta::sta_to_db_inst $inst]
      opendp::set_padding_inst $db_inst $left $right
    }
  }
}

sta::define_cmd_args "set_power_net" { [-power power_name]\
					 [-ground ground_net] }

proc set_power_net { args } {
  sta::parse_key_args "set_power_net" args \
    keys {-power -ground} flags {}

  sta::check_argc_eq0 "set_power_net" $args
  if { [info exists keys(-power)] } {
    set power $keys(-power)
    opendp::set_power_net_name $power
  }
  if { [info exists keys(-ground)] } {
    set ground $keys(-ground)
    opendp::set_ground_net_name $ground
  }
}

sta::define_cmd_args "filler_placement" { filler_masters }

proc filler_placement { args } {
  sta::check_argc_eq1 "filler_placement" $args
  set fillers [opendp::get_masters_arg "filler_masters" [lindex $args 0]]
  if { [llength $fillers] > 0 } {
    # pass master names for now
    set filler_names {}
    foreach filler $fillers {
      lappend filler_names [$filler getConstName]
    }
    opendp::filler_placement_cmd $filler_names
  }
}

sta::define_cmd_args "check_placement" {[-verbose]}

proc check_placement { args } {
  sta::parse_key_args "check_placement" args \
    keys {} flags {-verbose}

  set verbose [info exists flags(-verbose)]
  sta::check_argc_eq0 "check_placement" $args
  opendp::check_placement_cmd $verbose
}

sta::define_cmd_args "optimize_mirroring" {}

proc optimize_mirroring { args } {
  sta::check_argc_eq0 "optimize_mirroring" $args
  opendp::optimize_mirroring_cmd
}

namespace eval opendp {

proc get_masters_arg { arg_name arg } {
  set matched 0
  set masters {}
  # Look for liberty cells via get_lib_cells.
  set cells [sta::get_lib_cells_arg $arg_name $arg sta::warn]
  if { $cells != {} } {
    foreach cell $cells {
      set db_master [sta::sta_to_db_master $cell]
      lappend masters $db_master
      set matched 1
    }
  } else {
    # Expand master name regexps
    set db [ord::get_db]
    foreach name $arg {
      foreach lib [$db getLibs] {
	foreach master [$lib getMasters] {
	  set master_name [$master getConstName]
	  if { [regexp $name $master_name] } {
	    lappend masters $master
	    set matched 1
	  }
	}
      }
    }
  }
  if { !$matched } {
    puts "Warning: $name did not match any masters."
  }
  return $masters
}

proc get_inst_bbox { inst_name } {
  set block [ord::get_db_block]
  set inst [$block findInst $inst_name]
  if { $inst != "NULL" } {
    set bbox [$inst getBBox]
    return "[$bbox xMin] [$bbox yMin] [$bbox xMax] [$bbox yMax]"
  } else {
    error "cannot find instance $inst_name"
  }
}

proc get_inst_grid_bbox { inst_name } {
  set block [ord::get_db_block]
  set inst [$block findInst $inst_name]
  set rows [$block getRows]
  set site [[lindex $rows 0] getSite]
  set width [$site getWidth]
  set height [$site getHeight]
  if { $inst != "NULL" } {
    set bbox [$inst getBBox]
    return "[format_grid [$bbox xMin] $width] [format_grid [$bbox yMin] $height] [format_grid [$bbox xMax] $width] [format_grid [$bbox yMax] $height]"
  } else {
    error "cannot find instance $inst_name"
  }
}

proc format_grid { x w } {
  if { [expr $x % $w] == 0 } {
    return [expr $x / $w]
  } else {
    return [format "%.2f" [expr $x / double($w)]]
  }
}

}

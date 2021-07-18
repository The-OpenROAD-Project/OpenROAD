#############################################################################
##
## Copyright (c) 2019, The Regents of the University of California
## All rights reserved.
##
## BSD 3-Clause License
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

sta::define_cmd_args "detailed_placement" {[-max_displacement disp|{disp_x disp_y}]}

proc detailed_placement { args } {
  sta::parse_key_args "detailed_placement" args \
    keys {-max_displacement} flags {}

  if { [info exists keys(-max_displacement)] } {
    set max_displacement $keys(-max_displacement)
    if { [llength $max_displacement] == 1 } {
      sta::check_positive_integer "-max_displacement" $max_displacement
      set max_displacement_x $max_displacement
      set max_displacement_y $max_displacement
    } elseif { [llength $max_displacement] == 2 } {
      lassign $max_displacement max_displacement_x max_displacement_y
      sta::check_positive_integer "-max_displacement" $max_displacement_x
      sta::check_positive_integer "-max_displacement" $max_displacement_y
    } else {
      sta::error DPL 31 "-max_displacement disp|{disp_x disp_y}"
    }
  } else {
    # use default displacement
    set max_displacement_x 0
    set max_displacement_y 0
  }

  sta::check_argc_eq0 "detailed_placement" $args
  if { [ord::db_has_rows] } {
    set site [dpl::get_row_site]
    # Convert displacement from microns to sites.
    set max_displacement_x [expr [ord::microns_to_dbu $max_displacement_x] \
                              / [$site getWidth]]
    set max_displacement_y [expr [ord::microns_to_dbu $max_displacement_y] \
                              / [$site getHeight]]
    dpl::detailed_placement_cmd $max_displacement_x $max_displacement_y
    dpl::report_legalization_stats
  } else {
    utl::error "DPL" 27 "no rows defined in design. Use initialize_floorplan to add rows."
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
    dpl::set_padding_global $left $right
  } elseif { [info exists keys(-masters)] } {
    set masters [dpl::get_masters_arg "-masters" $keys(-masters)]
    foreach master $masters {
      dpl::set_padding_master $master $left $right
    }
  } elseif { [info exists keys(-instances)] } {
    # sta::get_instances_error supports sdc get_cells
    set insts [sta::get_instances_error "-instances" $keys(-instances)]
    foreach inst $insts {
      set db_inst [sta::sta_to_db_inst $inst]
      dpl::set_padding_inst $db_inst $left $right
    }
  }
}

sta::define_cmd_args "filler_placement" { [-prefix prefix] filler_masters }

proc filler_placement { args } {
  sta::parse_key_args "filler_placement" args \
    keys {-prefix} flags {}
  
  set prefix "FILLER_"
  if { [info exists keys(-prefix)] } {
    set prefix $keys(-prefix)
  }
  
  sta::check_argc_eq1 "filler_placement" $args
  set filler_masters [dpl::get_masters_arg "filler_masters" [lindex $args 0]]
  dpl::filler_placement_cmd $filler_masters $prefix
}

sta::define_cmd_args "check_placement" {[-verbose]}

proc check_placement { args } {
  sta::parse_key_args "check_placement" args \
    keys {} flags {-verbose}

  set verbose [info exists flags(-verbose)]
  sta::check_argc_eq0 "check_placement" $args
  return [dpl::check_placement_cmd $verbose]
}

sta::define_cmd_args "optimize_mirroring" {}

proc optimize_mirroring { args } {
  sta::check_argc_eq0 "optimize_mirroring" $args
  dpl::optimize_mirroring_cmd
}

namespace eval dpl {

proc get_masters_arg { arg_name arg } {
  set matched 0
  set masters {}
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
  if { !$matched } {
    utl::warn "DPL" 28 "$name did not match any masters."
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
    utl::error "DPL" 29 "cannot find instance $inst_name"
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
    utl::error "DPL" 30 "cannot find instance $inst_name"
  }
}

proc format_grid { x w } {
  if { [expr $x % $w] == 0 } {
    return [expr $x / $w]
  } else {
    return [format "%.2f" [expr $x / double($w)]]
  }
}

proc get_row_site {} {
  return [[lindex [[ord::get_db_block] getRows] 0] getSite]
}

}

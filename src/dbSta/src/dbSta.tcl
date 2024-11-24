############################################################################
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
##
############################################################################

namespace eval sta {
define_cmd_args "highlight_path" {[-min|-max] pin ^|r|rise|v|f|fall}

proc highlight_path { args } {
  parse_key_args "highlight_path" args keys {} \
    flags {-max -min} 0

  if { [ord::get_db_block] == "NULL" } {
    sta_error "No design block found."
  }

  if { [info exists flags(-min)] && [info exists flags(-max)] } {
    sta_error "-min and -max cannot both be specified."
  } elseif { [info exists flags(-min)] } {
    set min_max "min"
  } elseif { [info exists flags(-max)] } {
    set min_max "max"
  } else {
    # Default to max path.
    set min_max "max"
  }
  if { [llength $args] == 0 } {
    highlight_path_cmd "NULL"
  } else {
    check_argc_eq2 "highlight_path" $args

    set pin_arg [lindex $args 0]
    set tr [parse_rise_fall_arg [lindex $args 1]]

    set pin [get_port_pin_error "pin" $pin_arg]
    if { [$pin is_hierarchical] } {
      sta_error "pin '$pin_arg' is hierarchical."
    } else {
      foreach vertex [$pin vertices] {
        if { $vertex != "NULL" } {
          set worst_path [vertex_worst_arrival_path_rf $vertex $tr $min_max]
          if { $worst_path != "NULL" } {
            highlight_path_cmd $worst_path
            delete_path_ref $worst_path
          }
        }
      }
    }
  }
}

define_cmd_args "report_cell_usage" {[-verbose] [module_inst]}

proc report_cell_usage { args } {
  parse_key_args "highlight_path" args keys {} \
    flags {-verbose} 0

  check_argc_eq0or1 "report_cell_usage" $args

  if { [ord::get_db_block] == "NULL" } {
    sta_error 1001 "No design block found."
  }

  set module [[ord::get_db_block] getTopModule]
  if { $args != "" } {
    set modinst [[ord::get_db_block] findModInst $args]
    if { $modinst == "NULL" } {
      sta_error 1002 "Unable to find $args"
    }
    set module [$modinst getMaster]
  }

  report_cell_usage_cmd $module [info exists flags(-verbose)]
}

# redefine sta::sta_warn/error to call utl::warn/error
proc sta_error { id msg } {
  utl::error STA $id $msg
}

proc sta_warn { id msg } {
  utl::warn STA $id $msg
}

define_cmd_args "replace_design" {instance module}

proc replace_design { instance module } {
  set design [get_design_error $module]
  if { $design != "NULL" } {
    set inst [find_hier_inst_cmd $instance]
    replace_design_cmd $inst $design
    return 1
  }
  return 0
}

proc get_design_error { arg } {
  if { [llength $arg] > 1 } {
    sta_error 200 "module must be a single module."
  }
  set design [find_module_cmd $arg]
  if { $design == "NULL" } {
    sta_error 201 "module $arg cannot be found."
  }
  return $design
}

# namespace
}

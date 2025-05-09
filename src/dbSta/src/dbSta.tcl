# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

namespace eval sta {
define_cmd_args "report_cell_usage" { \
  [-verbose] [module_inst] [-file file] [-stage stage]}

proc report_cell_usage { args } {
  parse_key_args "highlight_path" args keys {-file -stage} \
    flags {-verbose} 0

  check_argc_eq0or1 "report_cell_usage" $args

  if { [ord::get_db_block] == "NULL" } {
    sta_error 1001 "No design block found."
  }

  set module [[ord::get_db_block] getTopModule]
  if { $args != "" } {
    set modinst [[ord::get_db_block] findModInst [lindex $args 0]]
    if { $modinst == "NULL" } {
      sta_error 1002 "Unable to find $args"
    }
    set module [$modinst getMaster]
  }
  set verbose [info exists flags(-verbose)]
  set file_name ""
  if { [info exists keys(-file)] } {
    set file_name $keys(-file)
  }
  set stage_name ""
  if { [info exists keys(-stage)] } {
    set stage_name $keys(-stage)
  }

  report_cell_usage_cmd $module $verbose $file_name $stage_name
}

define_cmd_args "report_timing_histogram" {[-num_bins num_bins] [-setup|-hold]}

proc report_timing_histogram { args } {
  parse_key_args "report_timing_histogram" args \
    keys {-num_bins} \
    flags {-setup -hold}

  check_argc_eq0 "report_timing_histogram" $args

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 7 "Both -setup and -hold cannot be specified"
  }

  set num_bins 10
  if { [info exists keys(-num_bins)] } {
    set num_bins $keys(-num_bins)
  }

  set min_max max
  if { [info exists flags(-hold)] } {
    set min_max min
  }

  report_timing_histogram_cmd $num_bins $min_max
}

define_cmd_args "report_logic_depth_histogram" { \
  [-num_bins num_bins] [-exclude_buffers] [-exclude_inverters]}

proc report_logic_depth_histogram { args } {
  parse_key_args "report_logic_depth_histogram" args keys \
    {-num_bins} flags {-exclude_buffers -exclude_inverters}

  check_argc_eq0 "report_logic_depth_histogram" $args

  set num_bins 10
  if { [info exists keys(-num_bins)] } {
    set num_bins $keys(-num_bins)
  }

  set exclude_buffers false
  if { [info exists flags(-exclude_buffers)] } {
    set exclude_buffers true
  }

  set exclude_inverters false
  if { [info exists flags(-exclude_inverters)] } {
    set exclude_inverters true
  }

  report_logic_depth_histogram_cmd $num_bins $exclude_buffers $exclude_inverters
}

# redefine sta::sta_warn/error to call utl::warn/error
proc sta_error { id msg } {
  utl::error STA $id $msg
}

proc sta_warn { id msg } {
  utl::warn STA $id $msg
}

define_cmd_args "replace_hier_module" {instance module}
proc replace_hier_module { instance module } {
  set design [get_hier_module $module]
  if { $design != "NULL" } {
    set modinst [[ord::get_db_block] findModInst $instance]
    if { $modinst == "NULL" } {
      sta_error 1003 "Unable to find $instance"
    }
    replace_hier_module_cmd $modinst $design
    return 1
  }
  return 0
}
interp alias {} replace_design {} replace_hier_module

define_cmd_args "get_hier_module" {design_name}
proc get_hier_module { arg } {
  if { [llength $arg] > 1 } {
    sta_error 200 "module must be a single module."
  }

  set block [ord::get_db_block]
  if { $block == "NULL" } {
    sta_error 202 "database block cannot be found."
  }

  set design [$block findModule $arg]
  if { $design == "NULL" } {
    set child_block [$block findChild $arg]
    if { $child_block != "NULL" } {
      set design [$child_block findModule $arg]
    }
  }

  if { $design == "NULL" } {
    sta_error 201 "module $arg cannot be found."
  }

  return $design
}
interp alias {} get_design {} get_hier_module

# namespace
}

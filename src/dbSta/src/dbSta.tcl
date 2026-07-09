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

define_cmd_args "report_timing_histogram" \
  {[-num_bins num_bins] [-bin_size bin_size] [-setup|-hold]}

proc report_timing_histogram { args } {
  parse_key_args "report_timing_histogram" args \
    keys {-num_bins -bin_size} \
    flags {-setup -hold}

  check_argc_eq0 "report_timing_histogram" $args

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 7 "Both -setup and -hold cannot be specified"
  }

  if { [info exists keys(-num_bins)] && [info exists keys(-bin_size)] } {
    utl::error STA 73 "Both -num_bins and -bin_size cannot be specified"
  }

  set num_bins 10
  if { [info exists keys(-num_bins)] } {
    set num_bins $keys(-num_bins)
  }

  set bin_size 0.0
  if { [info exists keys(-bin_size)] } {
    set bin_size $keys(-bin_size)
    if { $bin_size <= 0 } {
      utl::error STA 74 "-bin_size must be a positive value"
    }
  }

  set min_max max
  if { [info exists flags(-hold)] } {
    set min_max min
  }

  report_timing_histogram_cmd $num_bins $min_max $bin_size
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

define_cmd_args "check_axioms" {}
proc check_axioms { args } {
  check_argc_eq0 "check_axioms" $args
  check_axioms_cmd
}

proc endpoint_path_count { } {
  return [endpoint_count]
}

define_cmd_args "check_ip" {
  [-master master_name]
  [-all]
  [-max_polygons count]
  [-verbose]
}

proc check_ip { args } {
  parse_key_args "check_ip" args \
    keys {-master -max_polygons} \
    flags {-all -verbose}

  set master_name ""
  if { [info exists keys(-master)] } {
    set master_name $keys(-master)
  }

  set check_all [info exists flags(-all)]

  if { !$check_all && $master_name eq "" } {
    utl::error CHK 7 "Must specify either -master or -all"
  }

  set max_polygons 10000
  if { [info exists keys(-max_polygons)] } {
    set max_polygons $keys(-max_polygons)
    sta::check_positive_integer "-max_polygons" $max_polygons
  }

  set verbose [info exists flags(-verbose)]

  return [sta::check_ip_cmd $master_name $check_all $max_polygons $verbose]
}

################################################################
#
# Crosstalk / signal-integrity (SI) aware timing -- first slice.
#
################################################################

define_cmd_args "set_coupling_miller_factor" {[-setup mf_setup] [-hold mf_hold]}

# Apply a worst-case Miller coupling factor ("poor-man's SI") to the
# coupling caps kept in the parasitic network. -setup scales the max
# (setup) parasitics; -hold scales the min (hold) parasitics. Both default
# to 1.0, which reproduces baseline timing exactly. Typical bounding values:
# -setup 2.0 (worst-case opposite-switching aggressor) and -hold 0.0
# (best-case, aggressor quiet/same direction).
proc set_coupling_miller_factor { args } {
  parse_key_args "set_coupling_miller_factor" args \
    keys {-setup -hold} flags {}
  check_argc_eq0 "set_coupling_miller_factor" $args

  set mf_setup 1.0
  if { [info exists keys(-setup)] } {
    set mf_setup $keys(-setup)
    sta::check_positive_float "-setup" $mf_setup
  }
  set mf_hold 1.0
  if { [info exists keys(-hold)] } {
    set mf_hold $keys(-hold)
    # Hold may legitimately be 0.0 (best-case), so allow zero.
    if { ![string is double -strict $mf_hold] || $mf_hold < 0.0 } {
      sta_error 904 "-hold must be a non-negative float."
    }
  }

  sta::set_coupling_miller_factor_cmd $mf_setup $mf_hold
}

define_cmd_args "report_coupling_si" {[-max_nets count] [-corner index]}

# Rank signal nets by total coupling capacitance and report coupling cap,
# ground cap and coupling/ground ratio -- the nets most at SI risk.
proc report_coupling_si { args } {
  parse_key_args "report_coupling_si" args \
    keys {-max_nets -corner} flags {}
  check_argc_eq0 "report_coupling_si" $args

  set max_nets 20
  if { [info exists keys(-max_nets)] } {
    set max_nets $keys(-max_nets)
    sta::check_positive_integer "-max_nets" $max_nets
  }
  set corner 0
  if { [info exists keys(-corner)] } {
    set corner $keys(-corner)
    sta::check_positive_integer "-corner" $corner
  }

  sta::report_coupling_si_cmd $max_nets $corner
}

################################################################
# Depth-based (AOCV-style) OCV derate -- first slice (report-only).
# See AOCV_INVESTIGATION.md for design and limitations.

define_cmd_args "set_aocv_derate" { \
  [-file filename] \
  [-depth depth -late late_derate -early early_derate] \
  [-propagate] [-no_propagate] \
  [-reset] }

# Load a depth->derate table used by report_checks_aocv. Either supply a table
# file with -file, or add a single (depth late early) row inline. -reset clears
# the table (returns to inactive / baseline behavior).
#
# OpenROAD-fork: AOCV -- -propagate installs the table on the OpenSTA forward
# search so the depth derate is applied DURING timing propagation (real per-arc
# depth-dependent OCV), not just on reported paths. -no_propagate disables it
# again. Propagation defaults OFF, so without -propagate timing is identical to
# the baseline and only report_checks_aocv is affected.
proc set_aocv_derate { args } {
  parse_key_args "set_aocv_derate" args \
    keys {-file -depth -late -early} \
    flags {-reset -propagate -no_propagate}

  check_argc_eq0 "set_aocv_derate" $args

  if { [info exists flags(-propagate)] && [info exists flags(-no_propagate)] } {
    utl::error STA 8003 "set_aocv_derate: -propagate and -no_propagate are\
 mutually exclusive."
  }
  if { [info exists flags(-propagate)] } {
    sta::aocv_derate_set_propagate 1
  }
  if { [info exists flags(-no_propagate)] } {
    sta::aocv_derate_set_propagate 0
  }

  if { [info exists flags(-reset)] } {
    sta::aocv_derate_clear
    return
  }

  if { [info exists keys(-file)] } {
    set err [sta::aocv_derate_read_file $keys(-file)]
    if { $err ne "" } {
      utl::error STA 8000 "set_aocv_derate: $err"
    }
    return
  }

  if { [info exists keys(-depth)] } {
    if { ![info exists keys(-late)] } {
      utl::error STA 8001 "set_aocv_derate -depth requires -late."
    }
    set depth $keys(-depth)
    sta::check_positive_integer "-depth" $depth
    set late $keys(-late)
    set early $late
    if { [info exists keys(-early)] } {
      set early $keys(-early)
    }
    sta::aocv_derate_set_entry $depth $late $early
    return
  }

  # Allow `set_aocv_derate -propagate` / `-no_propagate` on their own (toggle
  # propagation of the already-loaded table) without requiring -file/-depth.
  if { [info exists flags(-propagate)] || [info exists flags(-no_propagate)] } {
    return
  }

  utl::error STA 8002 "set_aocv_derate: specify -file, -reset, -depth/-late,\
 or -propagate/-no_propagate."
}

define_cmd_args "report_checks_aocv" { \
  [-path_count count] \
  [-digits digits] \
  [find_timing_paths options] }

# Report, for the worst setup paths, the logic depth, the flat-OCV slack, the
# AOCV depth-adjusted slack, and the pessimism recovered. When no AOCV table is
# loaded the AOCV slack equals the flat slack exactly (feature inactive).
proc report_checks_aocv { args } {
  parse_key_args "report_checks_aocv" args \
    keys {-path_count -digits} \
    flags {} 0

  set digits 3
  if { [info exists keys(-digits)] } {
    set digits $keys(-digits)
    sta::check_positive_integer "-digits" $digits
  }

  # Default to worst setup paths; allow -path_count as a convenient alias for
  # find_timing_paths -group_path_count.
  set fp_args $args
  if { [info exists keys(-path_count)] } {
    sta::check_positive_integer "-path_count" $keys(-path_count)
    lappend fp_args -group_path_count $keys(-path_count)
  }

  set path_ends [eval find_timing_paths $fp_args]
  if { $path_ends == {} } {
    utl::report "No paths found."
    return
  }

  set active [sta::aocv_derate_active]
  if { !$active } {
    utl::report "Note: no AOCV table loaded (use set_aocv_derate);\
 AOCV slack == flat slack."
  }

  set w 14
  utl::report [format "%-40s %6s %*s %*s %*s %8s" \
    "Endpoint" "Depth" $w "FlatSlack" $w "AocvSlack" $w "Recovered" "Derate"]
  foreach path_end $path_ends {
    set row [sta::aocv_adjust_path_end $path_end]
    lassign $row endpoint depth flat aocv derate
    set recovered [expr { $aocv - $flat }]
    utl::report [format "%-40s %6d %*.*f %*.*f %*.*f %8.3f" \
      $endpoint $depth \
      $w $digits $flat \
      $w $digits $aocv \
      $w $digits $recovered \
      $derate]
  }
}

# namespace
}

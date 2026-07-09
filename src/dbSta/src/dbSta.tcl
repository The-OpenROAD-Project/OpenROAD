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

# Parametric statistical OCV (POCV / LVF) derate -- first slice (report-only).
# See POCV_INVESTIGATION.md for design, math, and limitations.

define_cmd_args "set_pocv_sigma" { \
  [-sigma per_stage_fraction] \
  [-n_sigma sigma_multiple] \
  [-reset] }

# Configure parametric POCV used by report_checks_pocv. -sigma is the per-stage
# fractional delay sigma (k), e.g. 0.05 for 5% 1-sigma per stage. -n_sigma is
# the sign-off sigma multiple (e.g. 3 for 3-sigma); defaults to 3 if omitted.
# -reset returns to inactive (POCV slack == flat slack). POCV is report-only and
# NEVER changes propagation/worst-slack timing (the forward search is untouched).
proc set_pocv_sigma { args } {
  parse_key_args "set_pocv_sigma" args \
    keys {-sigma -n_sigma} \
    flags {-reset}

  check_argc_eq0 "set_pocv_sigma" $args

  if { [info exists flags(-reset)] } {
    sta::pocv_sigma_clear
    return
  }

  if { ![info exists keys(-sigma)] } {
    utl::error STA 8010 "set_pocv_sigma: specify -sigma (per-stage fractional\
 sigma) or -reset."
  }
  set sigma $keys(-sigma)
  sta::check_positive_float "-sigma" $sigma

  set n_sigma 3.0
  if { [info exists keys(-n_sigma)] } {
    set n_sigma $keys(-n_sigma)
    sta::check_positive_float "-n_sigma" $n_sigma
  }
  sta::pocv_sigma_set $sigma $n_sigma
}

define_cmd_args "report_checks_pocv" { \
  [-path_count count] \
  [-digits digits] \
  [find_timing_paths options] }

# Report, for the worst setup paths, the logic depth, the flat-OCV slack, the
# POCV statistical (root-sum-square) slack, the linear vs RSS path sigma, and the
# pessimism recovered. The RSS path sigma grows as sqrt(depth) while the linear
# sigma grows as depth, so deeper paths recover proportionally more pessimism --
# the whole point of parametric POCV. With no sigma set, POCV slack == flat slack.
proc report_checks_pocv { args } {
  parse_key_args "report_checks_pocv" args \
    keys {-path_count -digits} \
    flags {} 0

  set digits 3
  if { [info exists keys(-digits)] } {
    set digits $keys(-digits)
    sta::check_positive_integer "-digits" $digits
  }

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

  if { ![sta::pocv_sigma_active] } {
    utl::report "Note: no POCV sigma set (use set_pocv_sigma);\
 POCV slack == flat slack."
  }

  set w 12
  utl::report [format "%-32s %5s %*s %*s %*s %*s %*s %7s" \
    "Endpoint" "Depth" $w "FlatSlack" $w "PocvSlack" $w "LinSigma" \
    $w "RssSigma" $w "Recovered" "Nsigma"]
  foreach path_end $path_ends {
    set row [sta::pocv_adjust_path_end $path_end]
    lassign $row endpoint depth flat pocv lin_sigma rss_sigma n_sigma
    set recovered [expr { $pocv - $flat }]
    utl::report [format "%-32s %5d %*.*f %*.*f %*.*f %*.*f %*.*f %7.2f" \
      $endpoint $depth \
      $w $digits $flat \
      $w $digits $pocv \
      $w $digits $lin_sigma \
      $w $digits $rss_sigma \
      $w $digits $recovered \
      $n_sigma]
  }
}

# namespace
}

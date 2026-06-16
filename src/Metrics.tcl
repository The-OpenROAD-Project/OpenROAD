# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2019-2025, The OpenROAD Authors

namespace eval sta {
define_cmd_args "report_clock_skew_metric" {[-setup]|[-hold]}
proc report_clock_skew_metric { args } {
  parse_key_args "report_clock_skew_metric" args keys {} flags {-setup -hold}

  set min_max "-setup"
  set metric_name "clock__skew__setup"
  if { ![info exists flags(-setup)] && [info exists flags(-hold)] } {
    set min_max "-hold"
    set metric_name "clock__skew__hold"
  } elseif { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error ORD 19 "both -setup and -hold specified."
  }

  utl::metric_float $metric_name [expr abs([worst_clock_skew $min_max])]
}

define_cmd_args "report_tns_metric" {[-setup]|[-hold]}
proc report_tns_metric { args } {
  parse_key_args "report_tns_metric" args keys {} flags {-setup -hold}

  set min_max "-max"
  set metric_name "timing__setup__tns"
  if { ![info exists flags(-setup)] && [info exists flags(-hold)] } {
    set min_max "-min"
    set metric_name "timing__hold__tns"
  } elseif { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error ORD 14 "both -setup and -hold specified."
  }

  utl::metric_float $metric_name [total_negative_slack $min_max]
}

define_cmd_args "report_worst_slack_metric" {[-setup]|[-hold]}
proc report_worst_slack_metric { args } {
  parse_key_args "report_worst_slack_metric" args keys {} flags {-setup -hold}

  set min_max "-max"
  set metric_name "timing__setup__ws"
  if { ![info exists flags(-setup)] && [info exists flags(-hold)] } {
    set min_max "-min"
    set metric_name "timing__hold__ws"
  } elseif { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error ORD 17 "both -setup and -hold specified."
  }

  utl::metric_float $metric_name [worst_slack $min_max]
}

define_cmd_args "report_worst_negative_slack_metric" {[-setup]|[-hold]}
proc report_worst_negative_slack_metric { args } {
  parse_key_args "report_worst_negative_slack_metric" args keys {} flags {-setup -hold}

  set min_max "-max"
  set metric_name "timing__setup__wns"
  if { ![info exists flags(-setup)] && [info exists flags(-hold)] } {
    set min_max "-min"
    set metric_name "timing__hold__wns"
  } elseif { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error ORD 18 "both -setup and -hold specified."
  }

  utl::metric_float $metric_name [worst_negative_slack $min_max]
}

define_cmd_args "report_fmax_metric" {}
proc report_fmax_metric { args } {
  parse_key_args "report_fmax_metric" args keys {} flags {}

  # Taken from: https://github.com/siliconcompiler/siliconcompiler/blob/e46c702df218c93483b951533fe00bcf01cf772d/siliconcompiler/tools/openroad/scripts/common/reports.tcl#L98
  # Modeled on: https://github.com/The-OpenROAD-Project/OpenSTA/blob/f913c3ddbb3e7b4364ed4437c65ac78c4da9174b/tcl/Search.tcl#L1078
  set fmax_metric 0
  foreach clk [sta::sort_by_name [all_clocks]] {
    set clk_name [get_name $clk]
    set min_period [sta::find_clk_min_period $clk 1]
    if { $min_period == 0.0 } {
      continue
    }
    set fmax [expr { 1.0 / $min_period }]
    utl::metric_float "timing__fmax__clock:${clk_name}" $fmax
    set fmax_metric [expr { max($fmax_metric, $fmax) }]
  }
  if { $fmax_metric == 0 } {
    # attempt to compute based on combinatorial path
    set fmax_valid true
    set max_paths [find_timing_paths -unconstrained -path_delay max]
    if { $max_paths == "" } {
      set fmax_valid false
    } else {
      set max_path_delay -1
      foreach path $max_paths {
        set path_delay [$path data_arrival_time]
        set max_path_delay [expr { max($max_path_delay, $path_delay) }]
      }
    }
    set min_paths [find_timing_paths -unconstrained -path_delay min]
    if { $min_paths == "" } {
      set fmax_valid false
    } else {
      set min_path_delay 1e12
      foreach path $min_paths {
        set path_delay [$path data_arrival_time]
        set min_path_delay [expr { min($min_path_delay, $path_delay) }]
      }
    }
    if { $fmax_valid } {
      set path_delay [expr { $max_path_delay - min(0, $min_path_delay) }]
      if { $path_delay > 0 } {
        set fmax_metric [expr { 1.0 / $path_delay }]
      }
    }
  }
  if { $fmax_metric > 0 } {
    utl::metric_float "timing__fmax" $fmax_metric
  }
}

# From https://wiki.tcl-lang.org/page/Inf
proc ::tcl::mathfunc::finite { x } {
  expr { [string is double -strict $x] && $x == $x && $x + 1 != $x }
}

define_cmd_args "report_erc_metrics" {}
proc report_erc_metrics { } {
  # Avoid tcl errors from division involving Inf
  if { [::tcl::mathfunc::finite [sta::max_slew_check_limit]] } {
    set max_slew_limit [sta::max_slew_check_slack_limit]
  } else {
    set max_slew_limit 0
  }
  if { [::tcl::mathfunc::finite [sta::max_capacitance_check_limit]] } {
    set max_cap_limit [sta::max_capacitance_check_slack_limit]
  } else {
    set max_cap_limit 0
  }
  if { [::tcl::mathfunc::finite [sta::max_fanout_check_limit]] } {
    set max_fanout_limit [sta::max_fanout_check_limit]
  } else {
    set max_fanout_limit 0
  }
  set max_slew_violation [sta::max_slew_violation_count]
  set max_cap_violation [sta::max_capacitance_violation_count]
  set max_fanout_violation [sta::max_fanout_violation_count]
  set setup_violation [sta::endpoint_violation_count max]
  set hold_violation [sta::endpoint_violation_count min]

  utl::metric_float "timing__drv__max_slew_limit" $max_slew_limit
  utl::metric_int "timing__drv__max_slew" $max_slew_violation
  utl::metric_float "timing__drv__max_cap_limit" $max_cap_limit
  utl::metric_int "timing__drv__max_cap" $max_cap_violation
  utl::metric_float "timing__drv__max_fanout_limit" $max_fanout_limit
  utl::metric_int "timing__drv__max_fanout" $max_fanout_violation
  utl::metric_int "timing__drv__setup_violation_count" $setup_violation
  utl::metric_int "timing__drv__hold_violation_count" $hold_violation
}


define_cmd_args "report_power_metric" {[-corner corner_name]}
proc report_power_metric { args } {
  parse_key_args "report_power_metric" args keys {-corner} flags {}
  set corner [sta::parse_scene keys]
  set power_result [design_power $corner]
  set totals [lrange $power_result 0 3]
  lassign $totals design_internal design_switching design_leakage design_total

  utl::metric_float "power__internal__total" $design_internal
  utl::metric_float "power__switching__total" $design_switching
  utl::metric_float "power__leakage__total" $design_leakage
  utl::metric_float "power__total" $design_total
}


define_cmd_args "report_units_metric" {}
proc report_units_metric { args } {
  utl::push_metrics_stage "run__flow__platform__{}_units"

  foreach unit {"time" "capacitance" "resistance" "voltage" "current" "power" "distance"} {
    utl::metric $unit "1[unit_scale_abbreviation $unit][unit_suffix $unit]"
  }

  utl::pop_metrics_stage
}


define_cmd_args "report_design_area_metrics" {}
proc report_design_area_metrics { args } {
  ord::report_design_area_metrics_cmd
}

# namespace
}

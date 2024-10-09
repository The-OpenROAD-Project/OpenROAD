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
  global sta_report_default_digits
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
  global sta_report_default_digits
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
  set corner [sta::parse_corner keys]
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
  set db [::ord::get_db]
  set dbu_per_uu [expr double([[$db getTech] getDbUnitsPerMicron])]
  set block [[$db getChip] getBlock]
  set die_bbox [$block getDieArea]
  set die_area [expr [$die_bbox dx] * [$die_bbox dy]]
  set core_bbox [$block getCoreArea]
  set core_area [expr [$core_bbox dx] * [$core_bbox dy]]

  set num_ios [llength [$block getBTerms]]

  set num_insts 0
  set num_stdcells 0
  set num_macros 0
  set num_padcells 0
  set num_cover 0

  set total_area 0.0
  set stdcell_area 0.0
  set macro_area 0.0
  set padcell_area 0.0
  set cover_area 0.0

  foreach inst [$block getInsts] {
    set inst_master [$inst getMaster]
    if { [$inst_master isFiller] } {
      continue
    }
    set wid [$inst_master getWidth]
    set ht [$inst_master getHeight]
    set inst_area [expr $wid * $ht]
    set total_area [expr $total_area + $inst_area]
    set num_insts [expr $num_insts + 1]

    if { [$inst_master isBlock] } {
      set num_macros [expr $num_macros + 1]
      set macro_area [expr $macro_area + $inst_area]
    } elseif { [$inst_master isCover] } {
      set num_cover [expr $num_cover + 1]
      set cover_area [expr $cover_area + $inst_area]
    } elseif { [$inst_master isPad] } {
      set num_padcells [expr $num_padcells + 1]
      set padcell_area [expr $padcell_area + $inst_area]
    } else {
      set num_stdcells [expr $num_stdcells + 1]
      set stdcell_area [expr $stdcell_area + $inst_area]
    }
  }

  set die_area [expr $die_area / [expr $dbu_per_uu * $dbu_per_uu]]
  set core_area [expr $core_area / [expr $dbu_per_uu * $dbu_per_uu]]
  set total_area [expr $total_area / [expr $dbu_per_uu * $dbu_per_uu]]
  set stdcell_area [expr $stdcell_area / [expr $dbu_per_uu * $dbu_per_uu]]
  set macro_area [expr $macro_area / [expr $dbu_per_uu * $dbu_per_uu]]
  set padcell_area [expr $padcell_area / [expr $dbu_per_uu * $dbu_per_uu]]
  set cover_area [expr $cover_area / [expr $dbu_per_uu * $dbu_per_uu]]

  set total_active_area [expr $stdcell_area + $macro_area]

  if { $core_area > 0 } {
    set core_util [expr $total_active_area / $core_area]
    if { $core_area > $macro_area } {
      set stdcell_util [expr $stdcell_area / [expr $core_area - $macro_area]]
    } else {
      set stdcell_util 0.0
    }
  } else {
    set core_util -1.0
    set stdcell_util -1.0
  }

  utl::metric_int "design__io" $num_ios
  utl::metric_float "design__die__area" $die_area
  utl::metric_float "design__core__area" $core_area
  utl::metric_int "design__instance__count" $num_insts
  utl::metric_float "design__instance__area" $total_active_area
  utl::metric_int "design__instance__count__stdcell" $num_stdcells
  utl::metric_float "design__instance__area__stdcell" $stdcell_area
  utl::metric_int "design__instance__count__macros" $num_macros
  utl::metric_float "design__instance__area__macros" $macro_area
  utl::metric_float "design__instance__utilization" $core_util
  utl::metric_float "design__instance__utilization__stdcell" $stdcell_util
}

# namespace
}

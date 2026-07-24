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
# report_qor: a single, read-only QoR (Quality of Results) sign-off summary.
#
# Aggregates metrics that OpenROAD already computes (design size, area /
# utilization, setup/hold timing, power, optional clock skew and DRC marker
# counts) into one concise dashboard. It recomputes nothing from scratch and
# changes no design or timing state -- every value comes from the read side of
# an existing command/API (see QOR_INVESTIGATION.md).
#
# Degrades gracefully: sections whose prerequisites are missing (no clock, no
# liberty, no parasitics/routing) print "n/a" instead of erroring. The command
# never crashes on a partially-set-up design.
#
define_cmd_args "report_qor" {[-digits digits] [-json]}

proc report_qor { args } {
  parse_key_args "report_qor" args keys {-digits} flags {-json}
  check_argc_eq0 "report_qor" $args

  set digits 3
  if { [info exists keys(-digits)] } {
    set digits $keys(-digits)
    check_positive_integer "-digits" $digits
  }
  set json [info exists flags(-json)]

  if { [ord::get_db_block] == "NULL" } {
    utl::error STA 90 "No design block found; cannot report QoR."
  }
  set block [ord::get_db_block]

  # ---- Design size (always available once a block is linked) ----
  set num_insts [llength [$block getInsts]]
  set num_nets [llength [$block getNets]]
  set num_iterms [llength [$block getITerms]]
  set num_bterms [llength [$block getBTerms]]
  set num_pins [expr { $num_iterms + $num_bterms }]

  # ---- Area / utilization (same engine as report_design_area) ----
  set area_um2 "n/a"
  set util_pct "n/a"
  if { ![catch { rsz::design_area } da] } {
    set area_um2 [expr { $da * 1e6 * 1e6 }]
  }
  if { ![catch { rsz::utilization } u] } {
    set util_pct [expr { $u * 100.0 }]
  }

  # ---- Timing (only when a clock is present and timing is set up) ----
  set have_clock [expr { [llength [sta::all_clocks]] > 0 }]
  set timing_ok 0
  array set tim {}
  if { $have_clock } {
    # Every call below is read-only; guard each so a partially constrained
    # design degrades to n/a rather than erroring.
    set timing_ok 1
    foreach {name expr_cmd} {
      setup_wns {sta::worst_negative_slack -max}
      setup_tns {sta::total_negative_slack -max}
      setup_ws  {sta::worst_slack -max}
      hold_wns  {sta::worst_negative_slack -min}
      hold_tns  {sta::total_negative_slack -min}
      hold_ws   {sta::worst_slack -min}
    } {
      if { [catch { eval $expr_cmd } v] } {
        set tim($name) "n/a"
        set timing_ok 0
      } else {
        set tim($name) $v
      }
    }
    foreach {name expr_cmd} {
      setup_viol {sta::endpoint_violation_count max}
      hold_viol  {sta::endpoint_violation_count min}
      endpoints  {sta::endpoint_count}
    } {
      if { [catch { eval $expr_cmd } v] } {
        set tim($name) "n/a"
      } else {
        set tim($name) $v
      }
    }
  }

  # ---- Power (only when liberty is loaded) ----
  set have_power [expr { [sta::liberty_libraries_exist] }]
  array set pwr {}
  if { $have_power } {
    if { [catch {
      set scene [sta::cmd_scene]
      set pr [sta::design_power $scene]
      lassign [lrange $pr 0 3] \
        pwr(internal) pwr(switching) pwr(leakage) pwr(total)
    }] } {
      set have_power 0
    }
  }

  # ---- Clock skew (optional; needs a clock) ----
  set skew_setup "n/a"
  set skew_hold "n/a"
  if { $have_clock } {
    catch { set skew_setup [sta::worst_clock_skew -setup] }
    catch { set skew_hold [sta::worst_clock_skew -hold] }
  }

  # ---- DRC / violation markers (optional; routed designs only) ----
  set drc_count "n/a"
  catch {
    set cnt 0
    set have_cats 0
    foreach cat [$block getMarkerCategories] {
      set have_cats 1
      incr cnt [llength [$cat getAllMarkers]]
    }
    if { $have_cats } {
      set drc_count $cnt
    }
  }

  if { $json } {
    qor_report_json $digits $num_insts $num_nets $num_pins \
      $area_um2 $util_pct $timing_ok [array get tim] \
      $have_power [array get pwr] $skew_setup $skew_hold $drc_count
  } else {
    qor_report_text $digits $num_insts $num_nets $num_pins $num_bterms \
      $area_um2 $util_pct $have_clock $timing_ok [array get tim] \
      $have_power [array get pwr] $skew_setup $skew_hold $drc_count
  }
}

# Format a numeric value to N digits, passing through non-numeric markers
# (e.g. "n/a") unchanged.
proc qor_fmt { v digits } {
  if { [string is double -strict $v] } {
    return [format "%.*f" $digits $v]
  }
  return $v
}

# Emit a JSON value: numbers formatted to N digits, "n/a"/missing as null.
proc qor_json_val { v digits } {
  if { [string is double -strict $v] } {
    return [format "%.*f" $digits $v]
  }
  return "null"
}

proc qor_report_text { digits num_insts num_nets num_pins num_bterms \
                       area_um2 util_pct have_clock timing_ok tim_list \
                       have_power pwr_list skew_setup skew_hold drc_count } {
  array set tim $tim_list
  array set pwr $pwr_list

  set lines {}
  lappend lines "==============================================="
  lappend lines "QoR Summary"
  lappend lines "==============================================="
  lappend lines "Design"
  lappend lines [format "  %-18s %s" "Instances:" $num_insts]
  lappend lines [format "  %-18s %s" "Nets:" $num_nets]
  lappend lines [format "  %-18s %s" "Pins:" $num_pins]
  lappend lines [format "  %-18s %s" "IO ports:" $num_bterms]
  lappend lines [format "  %-18s %s" "Area (um^2):" [qor_fmt $area_um2 $digits]]
  lappend lines [format "  %-18s %s" "Utilization (%):" [qor_fmt $util_pct $digits]]

  lappend lines "Timing"
  if { $have_clock && $timing_ok } {
    lappend lines [format "  %-18s WNS %s  TNS %s  WS %s" "Setup:" \
      [qor_fmt $tim(setup_wns) $digits] [qor_fmt $tim(setup_tns) $digits] \
      [qor_fmt $tim(setup_ws) $digits]]
    lappend lines [format "  %-18s WNS %s  TNS %s  WS %s" "Hold:" \
      [qor_fmt $tim(hold_wns) $digits] [qor_fmt $tim(hold_tns) $digits] \
      [qor_fmt $tim(hold_ws) $digits]]
    lappend lines [format "  %-18s setup %s  hold %s  (of %s endpoints)" \
      "Failing endpts:" $tim(setup_viol) $tim(hold_viol) $tim(endpoints)]
  } elseif { $have_clock } {
    lappend lines "  n/a (timing not fully set up)"
  } else {
    lappend lines "  n/a (no clock defined)"
  }

  lappend lines "Power"
  if { $have_power } {
    lappend lines [format "  %-18s %s" "Internal:" [qor_fmt $pwr(internal) $digits]]
    lappend lines [format "  %-18s %s" "Switching:" [qor_fmt $pwr(switching) $digits]]
    lappend lines [format "  %-18s %s" "Leakage:" [qor_fmt $pwr(leakage) $digits]]
    lappend lines [format "  %-18s %s" "Total:" [qor_fmt $pwr(total) $digits]]
  } else {
    lappend lines "  n/a (no liberty libraries)"
  }

  lappend lines "Clock"
  if { $have_clock } {
    lappend lines [format "  %-18s setup %s  hold %s" "Worst skew:" \
      [qor_fmt $skew_setup $digits] [qor_fmt $skew_hold $digits]]
  } else {
    lappend lines "  n/a (no clock defined)"
  }

  lappend lines "DRC"
  if { $drc_count == "n/a" } {
    lappend lines "  n/a (no markers; design not routed/checked)"
  } else {
    lappend lines [format "  %-18s %s" "Violations:" $drc_count]
  }
  lappend lines "==============================================="

  foreach line $lines {
    utl::report $line
  }
  return [join $lines "\n"]
}

proc qor_report_json { digits num_insts num_nets num_pins \
                       area_um2 util_pct timing_ok tim_list \
                       have_power pwr_list skew_setup skew_hold drc_count } {
  array set tim $tim_list
  array set pwr $pwr_list

  set lines {}
  lappend lines "{"
  lappend lines "  \"design\": {"
  lappend lines "    \"instances\": $num_insts,"
  lappend lines "    \"nets\": $num_nets,"
  lappend lines "    \"pins\": $num_pins,"
  lappend lines "    \"area_um2\": [qor_json_val $area_um2 $digits],"
  lappend lines "    \"utilization_pct\": [qor_json_val $util_pct $digits]"
  lappend lines "  },"

  if { $timing_ok } {
    lappend lines "  \"timing\": {"
    lappend lines "    \"setup_wns\": [qor_json_val $tim(setup_wns) $digits],"
    lappend lines "    \"setup_tns\": [qor_json_val $tim(setup_tns) $digits],"
    lappend lines "    \"setup_ws\": [qor_json_val $tim(setup_ws) $digits],"
    lappend lines "    \"hold_wns\": [qor_json_val $tim(hold_wns) $digits],"
    lappend lines "    \"hold_tns\": [qor_json_val $tim(hold_tns) $digits],"
    lappend lines "    \"hold_ws\": [qor_json_val $tim(hold_ws) $digits],"
    lappend lines "    \"setup_failing_endpoints\": [qor_json_val $tim(setup_viol) 0],"
    lappend lines "    \"hold_failing_endpoints\": [qor_json_val $tim(hold_viol) 0],"
    lappend lines "    \"endpoints\": [qor_json_val $tim(endpoints) 0]"
    lappend lines "  },"
  } else {
    lappend lines "  \"timing\": null,"
  }

  if { $have_power } {
    lappend lines "  \"power\": {"
    lappend lines "    \"internal\": [qor_json_val $pwr(internal) $digits],"
    lappend lines "    \"switching\": [qor_json_val $pwr(switching) $digits],"
    lappend lines "    \"leakage\": [qor_json_val $pwr(leakage) $digits],"
    lappend lines "    \"total\": [qor_json_val $pwr(total) $digits]"
    lappend lines "  },"
  } else {
    lappend lines "  \"power\": null,"
  }

  lappend lines "  \"clock\": {"
  lappend lines "    \"worst_skew_setup\": [qor_json_val $skew_setup $digits],"
  lappend lines "    \"worst_skew_hold\": [qor_json_val $skew_hold $digits]"
  lappend lines "  },"
  lappend lines "  \"drc_violations\": [qor_json_val $drc_count 0]"
  lappend lines "}"

  set out [join $lines "\n"]
  # utl::report runs its argument through fmt, which treats { and } as format
  # placeholders. Double them so the braces render literally; the returned
  # value keeps the real JSON for programmatic use.
  set safe [string map {\{ \{\{ \} \}\}} $out]
  utl::report $safe
  return $out
}

# namespace
}

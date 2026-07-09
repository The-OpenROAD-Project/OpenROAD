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

define_cmd_args "report_pba_slack" {[-max_paths count] [-setup] [-hold]\
                                      [-endpoints]}

# Path-Based Analysis pessimism-recovery report (additive diagnostic).
# For the top -max_paths GBA critical paths, reports GBA slack, PBA slack
# (after re-evaluating gate stages with path-specific slews) and the
# recovered pessimism (PBA slack - GBA slack, always >= 0). This does NOT
# change report_checks / GBA results.
#
# -setup (default) analyzes max (setup) paths; -hold analyzes min (hold)
# paths. -endpoints switches to the per-endpoint view with a
# negative->positive recovery summary. See AGENT_REPORT.md.
proc report_pba_slack { args } {
  parse_key_args "report_pba_slack" args \
    keys {-max_paths} flags {-setup -hold -endpoints}

  check_argc_eq0 "report_pba_slack" $args

  set max_paths 10
  if { [info exists keys(-max_paths)] } {
    set max_paths $keys(-max_paths)
    sta::check_positive_integer "-max_paths" $max_paths
  }

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2102 "report_pba_slack: -setup and -hold are mutually exclusive."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]

  if { [info exists flags(-endpoints)] } {
    sta::report_pba_endpoints_cmd $max_paths $min_max
  } else {
    sta::report_pba_slack_cmd $max_paths $min_max
  }
}

define_cmd_args "report_pba_closure" {[-max_paths count] [-setup] [-hold]\
                                        [-all]}

# PBA closure decision surface (additive diagnostic). Lists the endpoints
# that are STILL failing after PBA pessimism recovery (the genuine
# violations) and reports how many GBA-failing endpoints were merely
# GBA-pessimism artifacts cleared by PBA. Use -all to also list the
# recovered (artifact) endpoints. Does NOT change report_checks / GBA.
proc report_pba_closure { args } {
  parse_key_args "report_pba_closure" args \
    keys {-max_paths} flags {-setup -hold -all}

  check_argc_eq0 "report_pba_closure" $args

  set max_paths 10
  if { [info exists keys(-max_paths)] } {
    set max_paths $keys(-max_paths)
    sta::check_positive_integer "-max_paths" $max_paths
  }

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2103 "report_pba_closure: -setup and -hold are mutually exclusive."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]
  set only_violations [expr { [info exists flags(-all)] ? false : true }]

  sta::report_pba_closure_cmd $max_paths $min_max $only_violations
}

define_cmd_args "report_cppr" {[-max_paths count] [-setup] [-hold]}

# CPPR / CRPR -- Clock Reconvergence Pessimism Removal report (additive,
# report-only diagnostic). For the top -max_paths critical checks, reports:
#   * Raw slack    -- common clock-path pessimism still DOUBLE-COUNTED
#                     (PathEnd::slackNoCrpr); the conservative slack.
#   * CPPR slack   -- common-path credit applied (PathEnd::slack); this is
#                     the default GBA result reported by report_checks.
#   * Credit       -- pessimism credited back (CPPR slack - Raw slack >= 0).
#   * Common pin   -- the deepest shared clock pin (the branch point) the
#                     credit is attributed to.
# OpenSTA already implements CRPR and enables it by default under OCV
# analysis; this command SURFACES the raw-vs-adjusted split per check using
# the engine's own numbers. It does NOT change report_checks / GBA results
# and does NOT mutate the timing graph. -setup (default) analyzes max
# (setup) checks; -hold analyzes min (hold) checks.
proc report_cppr { args } {
  parse_key_args "report_cppr" args \
    keys {-max_paths} flags {-setup -hold}

  check_argc_eq0 "report_cppr" $args

  set max_paths 10
  if { [info exists keys(-max_paths)] } {
    set max_paths $keys(-max_paths)
    sta::check_positive_integer "-max_paths" $max_paths
  }

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2104 "report_cppr: -setup and -hold are mutually exclusive."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]

  sta::report_cppr_cmd $max_paths $min_max
}

define_cmd_args "report_cppr_closure" {[-max_paths count] [-setup] [-hold]\
                                         [-endpoints] [-all]}

# CPPR slice 2 -- closure decision surface (additive, report-only). Builds on
# report_cppr by aggregating the per-endpoint CPPR credit into a closure
# decision: endpoints failing under RAW GBA slack (common clock-path
# pessimism still double-counted) are classified into
#   * artifacts        -- pass once CPPR removes the double-counted pessimism
#                         (raw slack < 0 but CPPR-adjusted slack >= 0), and
#   * genuine          -- still fail after CPPR (CPPR-adjusted slack < 0).
# Mirrors report_pba_closure for cross-command consistency. With -endpoints
# the full per-endpoint table (raw / CPPR-adjusted slack, credit, status) is
# printed with a raw->cppr recovery summary; otherwise only the genuine
# (post-CPPR) violations are listed (-all also lists the cleared artifacts).
# Does NOT change report_checks / GBA results and does NOT mutate the graph.
# -setup (default) analyzes max (setup) checks; -hold analyzes min (hold).
proc report_cppr_closure { args } {
  parse_key_args "report_cppr_closure" args \
    keys {-max_paths} flags {-setup -hold -endpoints -all}

  check_argc_eq0 "report_cppr_closure" $args

  set max_paths 10
  if { [info exists keys(-max_paths)] } {
    set max_paths $keys(-max_paths)
    sta::check_positive_integer "-max_paths" $max_paths
  }

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2105 "report_cppr_closure: -setup and -hold are mutually exclusive."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]

  if { [info exists flags(-endpoints)] } {
    sta::report_cppr_endpoints_cmd $max_paths $min_max
  } else {
    set only_violations [expr { [info exists flags(-all)] ? false : true }]
    sta::report_cppr_closure_cmd $max_paths $min_max $only_violations
  }
}

define_cmd_args "report_mcmm_slack" {[-max_endpoints count] [-setup] [-hold]\
                                       [-by_mode]}

# MCMM -- Multi-Corner Multi-Mode cross-corner worst-slack report (additive,
# report-only diagnostic). For the top -max_endpoints critical endpoints,
# reports for each endpoint:
#   * the worst slack in EACH active corner (one column per corner),
#   * the WORST slack across all active corners, and
#   * the LIMITING corner name (the corner that produced the worst slack).
# Declare corners up front with `define_corners c1 c2 ...` and associate a
# liberty per corner with `read_liberty -corner <c> <file>`. OpenSTA already
# computes the cross-corner minimum; this command SURFACES it per endpoint in
# one auditable table using the engine's own numbers. It does NOT change
# report_checks / GBA results and does NOT mutate the timing graph. -setup
# (default) analyzes max (setup) checks; -hold analyzes min (hold) checks.
#
# Slice 2 (MODE dimension): with -by_mode, the limiting column also names the
# limiting MODE -- i.e. the true cross-mode x corner (mode, corner) pair -- and
# a per-mode breakdown section is printed (worst slack per endpoint within each
# mode's scenes). Modes/scenes are declared with the existing OpenSTA surface
# (`set_mode`, `define_scene -mode`, `read_sdc -mode`). Without -by_mode the
# output is byte-identical to slice 1.
proc report_mcmm_slack { args } {
  parse_key_args "report_mcmm_slack" args \
    keys {-max_endpoints} flags {-setup -hold -by_mode}

  check_argc_eq0 "report_mcmm_slack" $args

  set max_endpoints 10
  if { [info exists keys(-max_endpoints)] } {
    set max_endpoints $keys(-max_endpoints)
    sta::check_positive_integer "-max_endpoints" $max_endpoints
  }

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2106 "report_mcmm_slack: -setup and -hold are mutually exclusive."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]
  set by_mode [info exists flags(-by_mode)]

  sta::report_mcmm_slack_cmd $max_endpoints $min_max $by_mode
}

define_cmd_args "report_closure" {[-max_paths count] [-setup] [-hold] [-all]}

# Unified pessimism-recovery closure report (additive, report-only). The
# capstone of the PBA + CPPR closure surfaces: for every endpoint failing
# under the most pessimistic (pre-CPPR, pre-PBA) baseline, applies BOTH the
# CPPR common-path credit AND the PBA gate-slew recovery and classifies it:
#   * GENUINE  -- still failing after BOTH recoveries (the real violation),
#   * artifact -- cleared by pessimism recovery, labeled with WHICH mechanism
#                 cleared it (CPPR / PBA / BOTH).
# It COMPOSES the existing report_pba_closure and report_cppr_closure
# computations (it does not reimplement PBA or CPPR). Reports the combined
# recovered slack per endpoint plus a verdict summary (total / raw-failing /
# cleared-by-CPPR / cleared-by-PBA / cleared-by-both / genuine). By default
# only the genuine violations are listed; use -all to also list the cleared
# artifacts with their mechanism label. Does NOT change report_checks / GBA
# results and does NOT mutate the timing graph. -setup (default) analyzes max
# (setup) checks; -hold analyzes min (hold) checks.
proc report_closure { args } {
  parse_key_args "report_closure" args \
    keys {-max_paths} flags {-setup -hold -all}

  check_argc_eq0 "report_closure" $args

  set max_paths 10
  if { [info exists keys(-max_paths)] } {
    set max_paths $keys(-max_paths)
    sta::check_positive_integer "-max_paths" $max_paths
  }

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2107 "report_closure: -setup and -hold are mutually exclusive."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]
  set only_violations [expr { [info exists flags(-all)] ? false : true }]

  sta::report_closure_cmd $max_paths $min_max $only_violations
}

define_cmd_args "report_incremental_sta" \
  {-cells inst_list [-setup] [-hold]}

# Incremental STA -- prove that after a bounded ECO edit only the affected
# fanout cone needs re-evaluation, and surface the post-edit slacks.
#
# OpenSTA ALREADY recomputes timing incrementally after a netlist edit
# (resize / replace_cell / connect / disconnect): the dbSta ODB callbacks call
# the Sta edit hooks which mark only the affected vertices invalid, and the
# next timing query runs a levelized BFS that touches only the changed cone.
# This command is ADDITIVE and report-only: it does NOT invalidate timing and
# does NOT change report_checks / the full-STA path. It (a) identifies the
# affected fanout cone of the edited cells by walking the timing graph, (b)
# reports how many endpoints are in that cone vs. the full count, and (c)
# reports WNS/TNS and per-endpoint slacks from OpenSTA's own incremental query
# API -- which are identical to a full from-scratch re-run after the same
# edits. Apply the edits FIRST (e.g. with replace_cell), then run this.
# -setup (default) analyzes max (setup); -hold analyzes min (hold).
proc report_incremental_sta { args } {
  parse_key_args "report_incremental_sta" args \
    keys {-cells} flags {-setup -hold}

  check_argc_eq0 "report_incremental_sta" $args

  if { [info exists flags(-setup)] && [info exists flags(-hold)] } {
    utl::error STA 2108 "report_incremental_sta: -setup and -hold are mutually exclusive."
  }
  if { ![info exists keys(-cells)] } {
    utl::error STA 2109 "report_incremental_sta: -cells <inst_list> is required."
  }
  set min_max [expr { [info exists flags(-hold)] ? "min" : "max" }]

  sta::report_incremental_sta_cmd $keys(-cells) $min_max
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
#
# Crosstalk / SI -- SECOND slice: timing-window aware coupling derate.
# See SI_WINDOW_INVESTIGATION.md.
#
################################################################

define_cmd_args "set_si_timing_window" \
  {[-enable] [-disable] [-guardband seconds] [-max_nets count]}

# Turn window-aware coupling filtering on/off. When enabled, report_si_windows
# gates out aggressors whose switching window does not overlap the victim's,
# recovering the pessimism of the blanket Miller factor. Default (disabled, or
# never called) reproduces slice-1 / baseline timing exactly. -guardband widens
# each aggressor window by N seconds (more conservative); -max_nets limits the
# filter to the top-N highest-Cc victims (<=0 == all).
proc set_si_timing_window { args } {
  parse_key_args "set_si_timing_window" args \
    keys {-guardband -max_nets} flags {-enable -disable}
  check_argc_eq0 "set_si_timing_window" $args

  set enable 1
  if { [info exists flags(-disable)] } {
    set enable 0
  }
  if { [info exists flags(-enable)] && [info exists flags(-disable)] } {
    sta_error 913 "set_si_timing_window: -enable and -disable are mutually\
                   exclusive."
  }

  set guardband 0.0
  if { [info exists keys(-guardband)] } {
    set guardband $keys(-guardband)
    if { ![string is double -strict $guardband] || $guardband < 0.0 } {
      sta_error 914 "-guardband must be a non-negative float (seconds)."
    }
  }

  set max_nets 0
  if { [info exists keys(-max_nets)] } {
    set max_nets $keys(-max_nets)
    sta::check_positive_integer "-max_nets" $max_nets
  }

  sta::set_si_timing_window_cmd $enable $guardband $max_nets
}

define_cmd_args "report_si_windows" {[-max_nets count] [-corner index]}

# Report, per victim net: total coupling cap, window-filtered effective
# coupling cap, total aggressor segments, and # gated out (non-overlapping).
# When set_si_timing_window -enable is active this also applies the
# window-filtered coupling to the timing engine (scaling non-overlapping CC
# segs back toward nominal). When disabled it is a pure read-only what-if.
proc report_si_windows { args } {
  parse_key_args "report_si_windows" args \
    keys {-max_nets -corner} flags {}
  check_argc_eq0 "report_si_windows" $args

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

  sta::report_si_windows_cmd $max_nets $corner
}

################################################################
################################################################
#
# Crosstalk-aware TIMING -- coupling-cap effective-C stage-delay adjustment.
# See XTALK_DELAY_INVESTIGATION.md for the switching-factor model and limits.
#
# Turns the CC segments rcx already extracts into an OPTIONAL, flag-gated
# stage-delay adjustment: each top-N victim's coupling caps to aggressors that
# switch in the same timing window are bloated by (1 + k), so report_checks
# reflects the larger effective capacitance. DEFAULT (disabled / never called)
# == byte-identical baseline.
#
################################################################

define_cmd_args "set_xtalk_delay_factor" \
  {[-enable] [-disable] [-k factor] [-guardband seconds] [-max_nets count]\
   [-corner index] [-direction] [-iterations count] [-hold]}

# Enable/disable the crosstalk-aware effective-C delay adjustment. When enabled,
# each in-window CC segment of the top-N coupled victims has its stored coupling
# cap scaled by (1 + k); report_checks then sees the degraded stage delay.
# -disable (the default state) restores every bloated cap byte-identically ->
# exact baseline. k defaults to 1.0 (the classic ~2x Miller bound for a
# same-direction aggressor); -1 < k < 0 models a net-decoupling bound. k == 0
# is an explicit no-op. -guardband widens each aggressor window (more segments
# count as active); -max_nets limits to the top-N highest-Cc victims (<=0 all).
#
# Slice 2 (both default OFF -> byte-identical to slice 1):
#   -direction         classify each aggressor/victim pair by switching edge:
#                      same-direction -> effective-C up (1+k), opposite -> down
#                      (1-k, Miller decoupling). Off => slice-1 worst case (all
#                      active treated same-direction, factor 1+k).
#   -iterations count  run a small fixed number of bounded re-convergence
#                      passes (default 1 == slice-1 single pass) so aggressor
#                      windows settle as victim delays degrade. Per-pass TNS
#                      movement is reported.
#
# OpenROAD-fork: si-hold (default OFF -> byte-identical to the setup engine):
#   -hold              target the HOLD (min/early) path instead of setup. A
#                      same-direction aggressor switching with the victim
#                      REDUCES the effective coupling cap ((1-k) not (1+k)),
#                      speeding the victim and eroding hold slack; the
#                      adjustment is applied to the MIN parasitics. Off (the
#                      default) is the unchanged setup/max engine.
proc set_xtalk_delay_factor { args } {
  parse_key_args "set_xtalk_delay_factor" args \
    keys {-k -guardband -max_nets -corner -iterations} \
    flags {-enable -disable -direction -hold}
  check_argc_eq0 "set_xtalk_delay_factor" $args

  set enable 1
  if { [info exists flags(-disable)] } {
    set enable 0
  }
  if { [info exists flags(-enable)] && [info exists flags(-disable)] } {
    sta_error 940 "set_xtalk_delay_factor: -enable and -disable are mutually\
                   exclusive."
  }

  set k 1.0
  if { [info exists keys(-k)] } {
    set k $keys(-k)
    # k must be > -1 so the effective coupling cap (1 + k) stays non-negative.
    if { ![string is double -strict $k] || $k <= -1.0 } {
      sta_error 941 "-k must be a float > -1.0."
    }
  }

  set guardband 0.0
  if { [info exists keys(-guardband)] } {
    set guardband $keys(-guardband)
    if { ![string is double -strict $guardband] || $guardband < 0.0 } {
      sta_error 942 "-guardband must be a non-negative float (seconds)."
    }
  }

  set max_nets 0
  if { [info exists keys(-max_nets)] } {
    set max_nets $keys(-max_nets)
    sta::check_positive_integer "-max_nets" $max_nets
  }

  set corner 0
  if { [info exists keys(-corner)] } {
    set corner $keys(-corner)
    sta::check_positive_integer "-corner" $corner
  }

  # Slice-2 knobs; defaults reproduce slice-1 exactly.
  set direction 0
  if { [info exists flags(-direction)] } {
    set direction 1
  }

  set iterations 1
  if { [info exists keys(-iterations)] } {
    set iterations $keys(-iterations)
    sta::check_positive_integer "-iterations" $iterations
  }

  # OpenROAD-fork: si-hold. Default 0 -> setup engine (byte-identical).
  set hold 0
  if { [info exists flags(-hold)] } {
    set hold 1
  }

  sta::set_xtalk_delay_factor_cmd $enable $k $guardband $max_nets $corner \
    $direction $iterations $hold
}

define_cmd_args "report_xtalk_delay" \
  {[-k factor] [-guardband seconds] [-max_nets count] [-corner index]\
   [-direction] [-hold]}

# Report, per victim: total Cc, in-window (active) Cc, the added effective cap
# dC = k*Cc_active, the victim driver resistance, the implied stage-delay delta
# (Rdrv*dC), the # of active aggressors, the same/opposite-direction split, and
# whether it was applied. When set_xtalk_delay_factor -enable is active this
# re-applies the adjustment using the configured knobs; when disabled it is a
# read-only what-if using the knobs passed here (-k defaults to 1.0).
# -direction turns on per-edge direction classification for the what-if (when
# enabled, the configured direction setting is used instead).
proc report_xtalk_delay { args } {
  parse_key_args "report_xtalk_delay" args \
    keys {-k -guardband -max_nets -corner} flags {-direction -hold}
  check_argc_eq0 "report_xtalk_delay" $args

  set k 1.0
  if { [info exists keys(-k)] } {
    set k $keys(-k)
    if { ![string is double -strict $k] || $k <= -1.0 } {
      sta_error 943 "-k must be a float > -1.0."
    }
  }

  set guardband 0.0
  if { [info exists keys(-guardband)] } {
    set guardband $keys(-guardband)
    if { ![string is double -strict $guardband] || $guardband < 0.0 } {
      sta_error 944 "-guardband must be a non-negative float (seconds)."
    }
  }

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

  set direction 0
  if { [info exists flags(-direction)] } {
    set direction 1
  }

  # OpenROAD-fork: si-hold what-if. Default 0 -> setup report.
  set hold 0
  if { [info exists flags(-hold)] } {
    set hold 1
  }

  sta::report_xtalk_delay_cmd $k $guardband $max_nets $corner $direction $hold
}

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

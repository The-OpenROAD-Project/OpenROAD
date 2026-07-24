# Crosstalk-aware timing, slice 2: per-edge switching direction + bounded
# iterative re-convergence, on gcd (sky130hs) with real extracted coupling.
#
# Slice 1 assumed WORST-CASE same-direction coupling on every overlapping
# aggressor (effective coupling cap *= 1+k). Slice 2 adds, both flag-gated and
# both defaulting to the slice-1 behavior:
#
#   1. Per-edge DIRECTION: each victim/aggressor pair is classified by the
#      dominant overlapping switching edge. Same-polarity edges aggravate
#      (factor 1+k); opposite-polarity edges decouple (Miller, factor 1-k).
#      => the signed effective-cap delta is  k * (CcSame - CcOpp).
#   2. Bounded ITERATIONS: a small fixed number of refinement passes so the
#      aggressor arrival windows (which depend on aggressor delays, which
#      depend on coupling) settle. iterations=1 == slice-1 single pass.
#
# This test asserts, against a hand calculation on real extracted coupling:
#   (a) iterations=1 + direction OFF == slice-1 EXACTLY (no regression),
#   (b) a same-direction-dominated victim and an opposite-direction-dominated
#       victim get delay deltas of OPPOSITE SIGN,
#   (c) the signed delta magnitude matches  dDelay = Rdrv*k*(CcSame-CcOpp),
#       and CcSame + CcOpp == CcActive (the slice-1 active coupling cap),
#   (d) a victim with zero opposite-direction aggressors gives the SAME delta
#       with direction on or off (pure same-direction == slice-1),
#   (e) bounded re-convergence moves the setup TNS MONOTONICALLY and SETTLES
#       (per-pass movement is non-increasing and reaches a fixed point), and
#       iterations>=1 never makes timing optimistic vs a single pass.
#
# Coupling caps must be KEPT in the parasitic network
# (read_spef -keep_capacitive_coupling) for the adjustment to act.
source helpers.tcl

read_lef sky130hs/sky130hs.tlef
read_lef sky130hs/sky130hs_std_cell.lef
read_liberty sky130hs/sky130hs_tt.lib

read_def gcd.def
source sky130hs/sky130hs.rc

define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file ext_pattern.rules -max_res 0 \
  -coupling_threshold 0.1

set spef_file [make_result_file coupling_xtalk_direction.spef]
write_spef $spef_file
read_spef -keep_capacitive_coupling $spef_file

create_clock -name clk -period 1.0 [get_ports clk]

proc tns {} {
  return [sta::total_negative_slack_cmd "max"]
}

set fails 0
set k 0.5
set mn 25

# A same-direction-dominated victim and an opposite-direction-dominated victim
# in this extracted netlist (verified via report_xtalk_delay -direction).
set vic_same "_268_"
set vic_opp "_200_"

set base [tns]
puts "baseline TNS (xtalk-delay off): [format %.6e $base]"

# ---- (a) iterations=1 + direction OFF == slice-1 EXACTLY ----
# Slice-1 used the default enable path (single pass, no direction). Slice-2
# with explicit -iterations 1 and direction off must reproduce it bit-for-bit.
set_xtalk_delay_factor -enable -k $k -max_nets $mn
set slice1_tns [tns]
set_xtalk_delay_factor -disable

set_xtalk_delay_factor -enable -k $k -max_nets $mn -iterations 1
set s2_iter1_tns [tns]
set_xtalk_delay_factor -disable

if { $s2_iter1_tns == $slice1_tns } {
  puts "PASS: iterations=1 + direction off == slice-1 single pass exactly"
} else {
  puts "FAIL: slice-1 compat broken (slice1=$slice1_tns s2=$s2_iter1_tns)"
  incr fails
}

# ---- (b) same-dir vs opposite-dir victim: deltas have OPPOSITE sign ----
set dd_same [sta::xtalk_delay_ddelay_dir_for_net_cmd $vic_same $k 0.0 0]
set dd_opp  [sta::xtalk_delay_ddelay_dir_for_net_cmd $vic_opp  $k 0.0 0]
puts "dir dDelay: $vic_same = [format %.6e $dd_same]\
      $vic_opp = [format %.6e $dd_opp]"
if { $dd_same > 0.0 && $dd_opp < 0.0 } {
  puts "PASS: same-dir victim degrades (+), opposite-dir victim improves (-)"
} else {
  puts "FAIL: expected opposite signs (same=$dd_same opp=$dd_opp)"
  incr fails
}

# ---- (c) signed delta matches hand calc  dDelay = Rdrv*k*(CcSame-CcOpp) ----
# Recover Rdrv from the direction-OFF (slice-1) helper, where
# dDelay_off = Rdrv * k * CcActive, so Rdrv = dDelay_off / (k*CcActive*1e-15).
set dd_off  [sta::xtalk_delay_ddelay_for_net_cmd $vic_opp $k 0.0 0]
set cc_act  [sta::xtalk_delay_active_cc_for_net_cmd $vic_opp 0.0 0]
set cc_same [sta::xtalk_delay_dir_cc_for_net_cmd $vic_opp 0 0.0 0]
set cc_opp  [sta::xtalk_delay_dir_cc_for_net_cmd $vic_opp 1 0.0 0]
puts "$vic_opp: CcActive=[format %.4f $cc_act] CcSame=[format %.4f $cc_same]\
      CcOpp=[format %.4f $cc_opp]"

# CcSame + CcOpp must equal CcActive (every active segment is classified once).
if { abs(($cc_same + $cc_opp) - $cc_act) < 1e-9 } {
  puts "PASS: CcSame + CcOpp == CcActive (consistent classification)"
} else {
  puts "FAIL: CcSame+CcOpp ([expr {$cc_same+$cc_opp}]) != CcActive ($cc_act)"
  incr fails
}

if { $cc_act > 0.0 } {
  set rdrv [expr {$dd_off / ($k * $cc_act * 1e-15)}]
  set hand [expr {$rdrv * $k * ($cc_same - $cc_opp) * 1e-15}]
  set reldiff [expr {(abs($hand) > 0.0) ? abs($dd_opp - $hand) / abs($hand)
                                        : abs($dd_opp - $hand)}]
  puts "hand calc dDelay=[format %.6e $hand] engine=[format %.6e $dd_opp]\
        reldiff=[format %.3e $reldiff]"
  if { $reldiff < 1e-6 } {
    puts "PASS: signed dDelay matches Rdrv*k*(CcSame-CcOpp) hand calc"
  } else {
    puts "FAIL: dDelay does not match hand calc (reldiff=$reldiff)"
    incr fails
  }
} else {
  puts "FAIL: $vic_opp has no active coupling cap to hand-check"
  incr fails
}

# ---- (d) zero-opposite victim: direction on == direction off ----
# Find a victim whose active aggressors are ALL same-direction (CcOpp == 0);
# for it the direction-aware delta must equal the slice-1 (direction-off) delta
# exactly, because (1-k) is never applied.
set vic_pure ""
foreach n {net4 req_msg[5] req_msg[11] _049_} {
  set co [sta::xtalk_delay_dir_cc_for_net_cmd $n 1 0.0 0]
  set cs [sta::xtalk_delay_dir_cc_for_net_cmd $n 0 0.0 0]
  if { $co == 0.0 && $cs > 0.0 } {
    set vic_pure $n
    break
  }
}
if { $vic_pure ne "" } {
  set pd_off [sta::xtalk_delay_ddelay_for_net_cmd $vic_pure $k 0.0 0]
  set pd_dir [sta::xtalk_delay_ddelay_dir_for_net_cmd $vic_pure $k 0.0 0]
  puts "pure same-dir victim $vic_pure: off=[format %.6e $pd_off]\
        dir=[format %.6e $pd_dir]"
  if { $pd_dir == $pd_off } {
    puts "PASS: zero-opposite victim has direction on == off (no decoupling)"
  } else {
    puts "FAIL: zero-opposite victim differs (off=$pd_off dir=$pd_dir)"
    incr fails
  }
} else {
  puts "FAIL: no pure same-direction victim found to test"
  incr fails
}

# ---- (e) bounded re-convergence: monotonic + settles ----
# Enable with increasing iteration counts (idempotent: each enable restores
# first) and record the converged setup TNS. The sequence must move
# monotonically in one direction and reach a fixed point (the windows settle),
# with the per-pass movement non-increasing in magnitude.
set tns_seq {}
for { set it 1 } { $it <= 4 } { incr it } {
  set_xtalk_delay_factor -enable -k $k -max_nets $mn -direction -iterations $it
  lappend tns_seq [tns]
  set_xtalk_delay_factor -disable
}
puts "convergence TNS by iterations 1..4: $tns_seq"

# Monotonic: every step moves in the same (non-increasing slack) direction.
set mono 1
set settles 0
set prev_move ""
for { set i 1 } { $i < [llength $tns_seq] } { incr i } {
  set move [expr {[lindex $tns_seq $i] - [lindex $tns_seq [expr {$i - 1}]]}]
  # Direction tracking can only add effective coupling on net (TNS <= prev),
  # so each refinement pass must not improve TNS vs fewer passes.
  if { $move > 1e-18 } {
    set mono 0
  }
  if { abs($move) < 1e-18 } {
    set settles 1
  }
  if { $prev_move ne "" && abs($move) > abs($prev_move) + 1e-18 } {
    # movement should not grow as we add passes (settling, not diverging)
    set mono 0
  }
  set prev_move $move
}
if { $mono } {
  puts "PASS: re-convergence is monotonic with non-increasing movement"
} else {
  puts "FAIL: re-convergence not monotonic / movement grew ($tns_seq)"
  incr fails
}
if { $settles } {
  puts "PASS: re-convergence reaches a fixed point within 4 passes"
} else {
  puts "FAIL: re-convergence did not settle within 4 passes ($tns_seq)"
  incr fails
}

# Multi-pass must never be more optimistic than a single pass.
if { [lindex $tns_seq end] <= [lindex $tns_seq 0] + 1e-18 } {
  puts "PASS: converged TNS is not optimistic vs single pass"
} else {
  puts "FAIL: converged TNS optimistic vs single pass ($tns_seq)"
  incr fails
}

# ---- restore-to-baseline guard ----
set final [tns]
if { $final == $base } {
  puts "PASS: timing back at baseline after all xtalk-direction experiments"
} else {
  puts "FAIL: did not return to baseline (base=$base final=$final)"
  incr fails
}

if { $fails == 0 } {
  puts "pass"
} else {
  puts "FAIL: $fails check(s) failed"
}

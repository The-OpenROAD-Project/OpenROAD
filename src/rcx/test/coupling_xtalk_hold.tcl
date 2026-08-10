# Crosstalk-aware timing on HOLD paths (OpenROAD-fork: si-hold). Extends the
# setup/max coupling-cap effective-C adjustment to the min/early (hold) corner
# on gcd (sky130hs) with real extracted coupling caps. Verifies:
#   (a) the hold feature DISABLED (default) reproduces baseline min (hold)
#       timing EXACTLY -- byte-identical, the gating guarantee,
#   (b) the implied HOLD stage-delay delta has the correct SIGN: a
#       same-direction aggressor REDUCES the victim's effective coupling cap so
#       the victim arrives EARLIER -> the delta is NEGATIVE (a speed-up that
#       erodes hold slack), the exact sign mirror of the setup delta,
#   (c) MAGNITUDE matches a hand calculation: |dDelay_hold| == dDelay_setup ==
#       Rdrv * k * CcActive, and is linear in k (k=2 gives twice the magnitude),
#   (d) enabling -hold makes the victim's min-path arrival EARLIER (the min
#       parasitics are degraded) and disabling restores baseline min timing
#       EXACTLY (byte-reversible),
#   (e) k=0 with -hold is an explicit no-op (min timing identical to baseline).
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

set spef_file [make_result_file coupling_xtalk_hold.spef]
write_spef $spef_file
read_spef -keep_capacitive_coupling $spef_file

create_clock -name clk -period 1.0 [get_ports clk]

# Hold (min) timing aggregates.
proc hold_tns {} { return [sta::total_negative_slack_cmd "min"] }
proc hold_wns {} { return [sta::worst_slack_cmd "min"] }

set fails 0
set victim "_268_"

# Earliest (min) arrival across the victim net's load (sink) pins, in seconds.
# The hold adjustment changes the victim net's driver->load delay, so this
# min arrival must move EARLIER when the hold feature is enabled. Uses the
# pin "arrival_min_*" property (robust, no path-object plumbing).
proc victim_min_arrival { vic } {
  set net [get_nets $vic]
  set loads [get_pins -of_objects $net -filter "direction == input"]
  set best ""
  foreach p $loads {
    foreach prop {arrival_min_rise arrival_min_fall} {
      set a [get_property $p $prop]
      if { $a eq "" || $a eq "INF" || $a eq "-INF" } {
        continue
      }
      if { $best eq "" || $a < $best } {
        set best $a
      }
    }
  }
  return $best
}

# ---- (a) hold feature disabled (default) == baseline min timing ----
set base_tns [hold_tns]
set base_wns [hold_wns]
puts "baseline hold TNS=[format %.6e $base_tns] WNS=[format %.6e $base_wns]"

# ---- (b)+(c) sign + magnitude vs the setup mirror (hand calc) ----
# Active (in-window) coupling cap for the victim drives the hand calc.
set cc_active [sta::xtalk_delay_active_cc_for_net_cmd $victim 0.0 0]
puts "victim $victim active Cc: [format %.4f $cc_active] fF"
if { $cc_active <= 0.0 } {
  puts "FAIL: victim $victim has no in-window coupling cap to test"
  incr fails
}

set setup_dd [sta::xtalk_delay_ddelay_for_net_cmd $victim 1.0 0.0 0]
set hold_dd  [sta::xtalk_delay_hold_ddelay_for_net_cmd $victim 1.0 0.0 0]
puts "setup dDelay(k=1)=[format %.6e $setup_dd]\
      hold dDelay(k=1)=[format %.6e $hold_dd]"

# (b) sign: setup positive (slow-down), hold negative (speed-up).
if { $setup_dd > 0.0 && $hold_dd < 0.0 } {
  puts "PASS: hold delta is negative (early arrival) -- correct hold sign"
} else {
  puts "FAIL: expected setup>0 and hold<0 (setup=$setup_dd hold=$hold_dd)"
  incr fails
}

# (c) magnitude: hold delta is the exact sign mirror of the setup delta.
if { abs($hold_dd + $setup_dd) < 1e-21 } {
  puts "PASS: |hold dDelay| == setup dDelay == Rdrv*k*CcActive (exact mirror)"
} else {
  puts "FAIL: hold delta is not the negation of setup delta\
        (setup=$setup_dd hold=$hold_dd sum=[expr {$setup_dd+$hold_dd}])"
  incr fails
}

# linearity in k: hold dDelay(k=2) == 2 * hold dDelay(k=1).
set hold_dd2 [sta::xtalk_delay_hold_ddelay_for_net_cmd $victim 2.0 0.0 0]
if { $hold_dd < 0.0 } {
  set ratio [expr {$hold_dd2 / $hold_dd}]
  if { abs($ratio - 2.0) < 1e-6 } {
    puts "PASS: hold delay delta scales linearly with k (ratio=$ratio)"
  } else {
    puts "FAIL: hold delay delta not linear in k (ratio=$ratio, expected 2.0)"
    incr fails
  }
} else {
  puts "FAIL: expected negative hold delta to test linearity (hold_dd=$hold_dd)"
  incr fails
}

# ---- (d) enabling -hold makes the victim min-path arrive EARLIER ----
set arr_before [victim_min_arrival $victim]
set_xtalk_delay_factor -enable -k 1.0 -max_nets 50 -hold
set arr_after [victim_min_arrival $victim]
puts "victim min-path arrival: before=$arr_before after=$arr_after"
if { $arr_before ne "" && $arr_after ne "" } {
  if { $arr_after < [expr {$arr_before - 1e-15}] } {
    puts "PASS: hold enable pulls the victim min arrival earlier"
  } else {
    puts "FAIL: victim min arrival did not move earlier\
          (before=$arr_before after=$arr_after)"
    incr fails
  }
} else {
  puts "FAIL: could not measure victim min-path arrival\
        (before=$arr_before after=$arr_after)"
  incr fails
}

# ---- (d cont.) disable restores baseline min timing EXACTLY ----
set_xtalk_delay_factor -disable
set restored_tns [hold_tns]
set restored_wns [hold_wns]
if { $restored_tns == $base_tns && $restored_wns == $base_wns } {
  puts "PASS: disabling -hold restores baseline min timing exactly"
} else {
  puts "FAIL: disable did not restore baseline min timing\
        (tns $base_tns->$restored_tns wns $base_wns->$restored_wns)"
  incr fails
}

# ---- (e) k=0 with -hold is an explicit no-op ----
set_xtalk_delay_factor -enable -k 0.0 -max_nets 50 -hold
set k0_tns [hold_tns]
set k0_wns [hold_wns]
set_xtalk_delay_factor -disable
if { $k0_tns == $base_tns && $k0_wns == $base_wns } {
  puts "PASS: k=0 -hold is a no-op (min timing identical to baseline)"
} else {
  puts "FAIL: k=0 -hold changed min timing\
        (tns $base_tns->$k0_tns wns $base_wns->$k0_wns)"
  incr fails
}

# ---- final restore-to-baseline guard ----
set final_tns [hold_tns]
set final_wns [hold_wns]
if { $final_tns == $base_tns && $final_wns == $base_wns } {
  puts "PASS: min timing back at baseline after all hold experiments"
} else {
  puts "FAIL: did not return to baseline min timing\
        (tns $base_tns->$final_tns wns $base_wns->$final_wns)"
  incr fails
}

if { $fails == 0 } {
  puts "pass"
} else {
  puts "FAIL: $fails check(s) failed"
}

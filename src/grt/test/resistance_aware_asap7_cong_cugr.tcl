# CUGR resistance-aware on asap7 with the 2D maze engaged.
#
# Companion to resistance_aware_asap7_cugr (adjustment 0.25, maze idle):
# here adjustment 0.6 forces congestion that pattern routing can't resolve,
# so Stage 3 (the 2D maze + iterative RRR) runs. It still converges to 0
# congestion, so timing is meaningful. This exercises resistance-aware on
# the maze path, where net ordering matters.
#
# Net ordering note: critical nets are routed first in every stage,
# including the maze. An A/B experiment (critical-first vs critical-last in
# the maze) found critical-first better-or-equal: identical here at 0.6 and
# clearly better under heavier congestion (a rip-up/reroute gives the
# last-routed nets the worst leftovers). Preserving a critical net's
# topology is the freeze's job, not the ordering's.
#
# Baseline on this design WITHOUT -resistance_aware
# (global_route -use_cugr -critical_nets_percentage 30):
#   WNS = -122.0 ps, TNS = -2912.8 ps
# Resistance-aware gives ~ WNS -121.9 ps, TNS -2879.7 ps (TNS ~33 ps
# better; WNS within noise). The WNS baseline carries a small margin so
# the check is not brittle to sub-ps drift.
source "helpers.tcl"

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_AO_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_AO_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def.gz
read_sdc gcd_asap7.sdc

source asap7/setRC.tcl
set_wire_rc -signal -layer M3
set_wire_rc -clock -layer M6
set_propagated_clock [all_clocks]
estimate_parasitics -placement

set guide_file [make_result_file resistance_aware_asap7_cong_cugr.guide]

set_routing_layers -signal M2-M6 -clock M4-M6
set_global_routing_layer_adjustment M2-M7 0.6

global_route -use_cugr -critical_nets_percentage 30 -resistance_aware

write_guides $guide_file

estimate_parasitics -global_routing

set baseline_wns -122.0
set baseline_tns -2912.8
set wns [worst_slack -max]
set tns [total_negative_slack -max]
puts "baseline (no resistance-aware) WNS: [format %.1f $baseline_wns] ps  TNS: [format %.1f $baseline_tns] ps"
puts "resistance-aware               WNS: [format %.1f $wns] ps  TNS: [format %.1f $tns] ps"
if { $wns >= $baseline_wns && $tns >= $baseline_tns } {
  puts "resistance-aware WNS and TNS not worse than baseline: PASS"
} else {
  puts "resistance-aware timing worse than baseline: FAIL"
}

diff_file resistance_aware_asap7_cong_cugr.guideok $guide_file

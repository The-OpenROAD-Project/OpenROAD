# FastRoute resistance-aware on asap7 (reference / counterpart to
# resistance_aware_asap7_cong_cugr, which routes the same design with CUGR).
#
# Same gcd_asap7 design and adjustment (0.6) as the CUGR maze-engaged test,
# but with the default FastRoute engine (no -use_cugr). Confirms FR's
# resistance-aware path improves timing here (it handles critical nets via
# net ordering + a resistance cost in its 3D maze / layer assignment), and
# gives a head-to-head reference for the CUGR numbers.
#
# Baseline on this design WITHOUT -resistance_aware
# (global_route -critical_nets_percentage 30):
#   WNS = -122.7 ps, TNS = -2957.2 ps
# Resistance-aware gives ~ WNS -122.3 ps, TNS -2916.8 ps (both better).
# For reference, CUGR on the same design: OFF TNS -2912.8, ON -2879.7 ps
# (CUGR routes this design with better absolute timing than FR).
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

set guide_file [make_result_file resistance_aware_asap7_fr.guide]

set_routing_layers -signal M2-M6 -clock M4-M6
set_global_routing_layer_adjustment M2-M7 0.6

global_route -critical_nets_percentage 30 -resistance_aware

write_guides $guide_file

estimate_parasitics -global_routing

set baseline_wns -122.7
set baseline_tns -2957.2
set wns [worst_slack -max]
set tns [total_negative_slack -max]
set base_wns [format %.1f $baseline_wns]
set base_tns [format %.1f $baseline_tns]
set ra_wns [format %.1f $wns]
set ra_tns [format %.1f $tns]
puts "baseline (no resistance-aware) WNS: $base_wns ps  TNS: $base_tns ps"
puts "resistance-aware               WNS: $ra_wns ps  TNS: $ra_tns ps"
if { $wns >= $baseline_wns && $tns >= $baseline_tns } {
  puts "resistance-aware WNS and TNS not worse than baseline: PASS"
} else {
  puts "resistance-aware timing worse than baseline: FAIL"
}

diff_file resistance_aware_asap7_fr.guideok $guide_file

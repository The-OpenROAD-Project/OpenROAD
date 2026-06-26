# CUGR resistance-aware routing on a wire-resistance-limited PDK (asap7).
#
# Unlike sky130hs (cell-delay-limited), asap7 has a steep per-unit wire
# resistance gradient (M1 7.04e-2 -> M6 1.18e-2, ~6x) and thin 7nm wires,
# so biasing critical nets onto lower-resistance upper metals measurably
# improves timing. This is the timing-validation test for resistance-aware:
# it reports WNS/TNS after global routing and checks that res-aware TNS is
# no worse than the (documented) non-res-aware baseline on the same design.
#
# Baseline measured on this design with the identical setup but WITHOUT
# -resistance_aware (global_route -use_cugr -critical_nets_percentage 30):
#   WNS = -121.9 ps, TNS = -2892.8 ps
# Resistance-aware gives ~ WNS -121.7 ps, TNS -2852.3 ps (both better).
# A single global_route cannot be re-run in one session (GRT-0076), so the
# baseline is captured here as a constant rather than routed twice. The WNS
# baseline carries a small margin (the WNS gain is only ~0.15 ps) so the
# "not worse" check is not brittle to sub-ps STA drift.
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

set guide_file [make_result_file resistance_aware_asap7_cugr.guide]

set_routing_layers -signal M2-M6 -clock M4-M6
set_global_routing_layer_adjustment M2-M7 0.25

global_route -use_cugr -critical_nets_percentage 30 -resistance_aware

write_guides $guide_file

# Timing after routing-based parasitics: this is where resistance-aware
# pays off (critical nets sit on lower-resistance upper metals).
estimate_parasitics -global_routing

set baseline_wns -121.9
set baseline_tns -2892.8
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

diff_file resistance_aware_asap7_cugr.guideok $guide_file

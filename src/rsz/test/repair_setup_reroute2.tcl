# repair_timing -setup with the REROUTE phase on ASAP7 GCD.
# The REROUTE phase applies RerouteMove to high-wire-delay nets on violating
# paths through the Reroute generator/candidate flow.
# Requires global routing to be present (haveRoutes).
source "helpers.tcl"

suppress_message STA 1212

# RVT FF libs
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
# LVT FF libs
read_liberty asap7/asap7sc7p5t_AO_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
# SLVT FF libs
read_liberty asap7/asap7sc7p5t_AO_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib

read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef

read_def gcd_asap7_placed_sparse.def
read_sdc gcd.sdc

source asap7/setRC.tcl
set_routing_layers -signal M2-M6 -clock M4-M6
global_route
estimate_parasitics -global_routing

set wns_before [worst_slack -max]
set tns_before [total_negative_slack -max]
check "gcd starts with setup violations" { expr { $wns_before < 0 } } 1

tee -quiet -variable log { repair_timing -setup -phases "REROUTE" -repair_tns 10 }

check "repair_timing reroutes nets resistance-aware" {
  expr { [regexp {RSZ-0053\] Rerouted ([0-9]+) nets} $log -> nets] && $nets > 0 }
} 1
check "rerouting improves WNS" { expr { [worst_slack -max] > $wns_before } } 1
check "rerouting improves TNS" {
  expr { [total_negative_slack -max] > $tns_before }
} 1

exit_summary

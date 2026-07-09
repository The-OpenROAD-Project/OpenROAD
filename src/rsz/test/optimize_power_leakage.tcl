# optimize_power -leakage on a multi-Vt (asap7 RVT/LVT/SLVT) placed design.
# Verifies leakage power decreases and WNS/TNS do not degrade.
source "helpers.tcl"
source asap7/asap7.vars

proc capture_ppa { } {
  set scene [sta::cmd_scene]
  set wns [sta::worst_slack_cmd max]
  set tns [sta::total_negative_slack_cmd max]
  lassign [sta::design_power $scene] _internal _switching leakage _total
  return [list $wns $tns $leakage]
}

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

read_def gcd_asap7_placed.def
read_sdc gcd.sdc
source asap7/setRC.tcl
estimate_parasitics -placement

lassign [capture_ppa] wns_before tns_before leakage_before
optimize_power -leakage
lassign [capture_ppa] wns_after tns_after leakage_after

# Assertions: leakage strictly down; WNS/TNS not worse (allow tiny jitter).
set tol 1e-15
if { $leakage_after < $leakage_before } {
  puts "LEAKAGE: decreased (PASS)"
} else {
  puts "LEAKAGE: did NOT decrease (FAIL) before=$leakage_before after=$leakage_after"
}
if { $wns_after >= [expr { $wns_before - $tol }] } {
  puts "WNS: not degraded (PASS)"
} else {
  puts "WNS: degraded (FAIL) before=$wns_before after=$wns_after"
}
if { $tns_after >= [expr { $tns_before - $tol }] } {
  puts "TNS: not degraded (PASS)"
} else {
  puts "TNS: degraded (FAIL) before=$tns_before after=$tns_after"
}

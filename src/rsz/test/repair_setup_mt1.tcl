# repair_setup Mt1 VT-swap policy smoke on multi-VT ASAP7 design
source "helpers.tcl"
source asap7/asap7.vars

set ::env(RSZ_POLICY) vtswapmt1
set ::env(RSZ_VTSWAP_CANDIDATES) 10
set ::env(RSZ_VTSWAP_MAX_MOVES) 100

proc capture_setup_ppa { } {
  set scene [sta::cmd_scene]
  set wns [sta::worst_slack_cmd max]
  if { $wns > 0.0 } {
    set wns 0.0
  }
  set tns [sta::total_negative_slack_cmd max]
  set area_um2 [expr { [rsz::design_area] * 1e12 }]
  lassign [sta::design_power $scene] _internal _switching _leakage total_power
  return [list $wns $tns $area_um2 $total_power]
}

proc report_setup_ppa_delta { before after } {
  lassign $before before_wns before_tns before_area before_power
  lassign $after after_wns after_tns after_area after_power

  set ppa_row_format "%-10s WNS=%8s   TNS=%8s   Area=%8.0f um^2   Power=% 12.6e W"
  set delta_row_format "%-10s WNS=%8s   TNS=%8s   Area=%+8.0f um^2   Power=%+12.6e W"

  puts [format \
    $ppa_row_format \
    "Pre-ECO:" \
    [sta::format_time $before_wns 3] \
    [sta::format_time $before_tns 3] \
    $before_area \
    $before_power]
  puts [format \
    $ppa_row_format \
    "Post-ECO:" \
    [sta::format_time $after_wns 3] \
    [sta::format_time $after_tns 3] \
    $after_area \
    $after_power]
  puts [format \
    $delta_row_format \
    "Delta:" \
    [sta::format_time [expr { $after_wns - $before_wns }] 3] \
    [sta::format_time [expr { $after_tns - $before_tns }] 3] \
    [expr { $after_area - $before_area }] \
    [expr { $after_power - $before_power }]]
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

set ppa_before [capture_setup_ppa]
repair_timing -setup -skip_last_gasp -verbose
set ppa_after [capture_setup_ppa]
report_setup_ppa_delta $ppa_before $ppa_after

puts "pass"

unset -nocomplain ::env(RSZ_POLICY)
unset -nocomplain ::env(RSZ_VTSWAP_CANDIDATES)
unset -nocomplain ::env(RSZ_VTSWAP_MAX_MOVES)

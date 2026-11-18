# VT swap after unset_dont_use without buffer pruning: the library VT data
# must follow the dont_use change (one usable VT, then three)
source "helpers.tcl"
foreach vt {RVT LVT SLVT} {
  read_liberty asap7/asap7sc7p5t_AO_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_INVBUF_${vt}_FF_nldm_220122.lib.gz
  read_liberty asap7/asap7sc7p5t_OA_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_SIMPLE_${vt}_FF_nldm_211120.lib.gz
  read_liberty asap7/asap7sc7p5t_SEQ_${vt}_FF_nldm_220123.lib
}
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def
read_sdc gcd.sdc
source asap7/setRC.tcl
estimate_parasitics -placement
set_opt_config -disable_buffer_pruning true

# Instance count per VT suffix
proc report_vt_counts { tag } {
  set counts [dict create R 0 L 0 SL 0]
  foreach inst [get_cells *] {
    if { [regexp {_ASAP7_75t_(R|L|SL)$} [get_property $inst ref_name] -> vt] } {
      dict incr counts $vt
    }
  }
  puts "$tag: R [dict get $counts R] L [dict get $counts L] SL [dict get $counts SL]"
}

# First repair with RVT as the only usable VT
set_dont_use [get_lib_cells {*_ASAP7_75t_L *_ASAP7_75t_SL}]
report_vt_counts "before"
repair_timing -setup -sequence vt_swap -max_iterations 2
report_vt_counts "RVT only"

# Second repair with every VT usable: cells move to the faster VTs
unset_dont_use [get_lib_cells {*_ASAP7_75t_L *_ASAP7_75t_SL}]
repair_timing -setup -sequence vt_swap -max_iterations 2
report_vt_counts "all VTs"
report_worst_slack -max -digits 3

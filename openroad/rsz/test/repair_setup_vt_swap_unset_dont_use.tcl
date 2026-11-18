# VT swap after unset_dont_use: cells must reach the VT that was dont_use
# during an earlier repair_timing (getVTEquivCells must not keep its old list)
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

# First repair with SLVT on dont_use: RVT cells can only move to LVT
set_dont_use [get_lib_cells *_ASAP7_75t_SL]
report_vt_counts "before"
repair_timing -setup -phases PARALLEL_EVAL -sequence vt_swap -max_iterations 2
report_vt_counts "SLVT dont_use"

# Second repair with SLVT usable: cells move on to SLVT
unset_dont_use [get_lib_cells *_ASAP7_75t_SL]
repair_timing -setup -phases PARALLEL_EVAL -sequence vt_swap -max_iterations 2
report_vt_counts "SLVT usable"
report_worst_slack -max -digits 3

# LEGACY VtSwap with one usable VT: cells mapped to a dont_use VT (here RVT
# and LVT, SLVT is the only usable VT) move to the usable VT.
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
read_def gcd_asap7_placed.def
read_sdc gcd.sdc
source asap7/setRC.tcl

# SLVT is the only usable VT.
set_dont_use [get_lib_cells {*_ASAP7_75t_R *_ASAP7_75t_L}]
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

report_vt_counts "before"
repair_timing -setup \
  -sequence vt_swap \
  -max_iterations 2 \
  -verbose
report_vt_counts "after"
report_worst_slack -max -digits 3

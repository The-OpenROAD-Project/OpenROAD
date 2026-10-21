# Critical-path slice extracted from asap7.  Drives LEGACY repair_setup.

source "helpers.tcl"

foreach lib {
  asap7sc7p5t_AO_LVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_AO_SLVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
  asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
  asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
  asap7sc7p5t_OA_LVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_OA_SLVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_SIMPLE_SLVT_FF_nldm_211120.lib.gz
  asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
  asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
  asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib
} {
  read_liberty asap7/$lib
}
foreach lef {
  asap7_tech_1x_201209.lef
  asap7sc7p5t_28_R_1x_220121a.lef
  asap7sc7p5t_28_L_1x_220121a.lef
  asap7sc7p5t_28_SL_1x_220121a.lef
} {
  read_lef asap7/$lef
}
read_def repair_setup_path4.def
read_sdc repair_setup_path4.sdc

source asap7/setRC.tcl
estimate_parasitics -placement

repair_timing -setup -phases "LEGACY"
estimate_parasitics -placement
report_checks -path_delay max -format full -digits 4

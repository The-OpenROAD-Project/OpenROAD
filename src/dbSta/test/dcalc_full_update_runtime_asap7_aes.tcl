# Compare full timing-update runtime on the placed ASAP7 AES design.
set test_name dcalc_full_update_runtime_asap7_aes
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_def ../../psm/test/asap7_data/aes_place.def
read_sdc ../../psm/test/asap7_data/aes_place.sdc

source asap7/setRC.tcl
estimate_parasitics -placement

run_dcalc_full_update_runtime

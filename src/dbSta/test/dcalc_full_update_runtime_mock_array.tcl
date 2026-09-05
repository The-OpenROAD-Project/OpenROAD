# Compare full timing-update runtime on the hierarchical ASAP7 MockArray design.
set test_name dcalc_full_update_runtime_mock_array
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_verilog MockArray.v
read_verilog Element.v
link_design -hier MockArray
read_sdc MockArray.sdc
read_spef MockArray.spef
read_spef -path ces_0_0 Element.spef

run_dcalc_full_update_runtime

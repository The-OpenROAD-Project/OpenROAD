# Compare full timing-update runtime on the placed ASAP7 RISC-V design.
set test_name dcalc_full_update_runtime_asap7_riscv
source "helpers.tcl"
source "dcalc_full_update_runtime_helpers.tcl"

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/fakeram7_256x32.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/fakeram7_256x32.lef
read_def ../../psm/test/asap7_data/riscv.def

set clk_port [get_ports clk]
set non_clock_inputs [all_inputs -no_clocks]
create_clock -name clk -period 3000 $clk_port
set_input_delay 0 -clock clk $non_clock_inputs
set_output_delay 0 -clock clk [all_outputs]

source asap7/setRC.tcl
estimate_parasitics -placement

run_dcalc_full_update_runtime

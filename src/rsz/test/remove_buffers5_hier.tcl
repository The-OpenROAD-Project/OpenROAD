# remove_buffers test w/ asap7/riscv32i design in hierarchical flow
source "helpers.tcl"

set test_name remove_buffers5_hier

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/fakeram7_256x32.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/fakeram7_256x32.lef

read_verilog asap7_riscv32i.v
link_design riscv_top

set clk_name clk
set clk_port_name clk
set clk_period 3000
set clk_port [get_ports $clk_port_name]
set non_clock_inputs [lsearch -inline -all -not -exact [all_inputs] $clk_port]

create_clock -name $clk_name -period $clk_period $clk_port
set_input_delay 0 -clock $clk_name $non_clock_inputs
set_output_delay 0 -clock $clk_name [all_outputs]

# make sure sta works before/after removal
set report_checks_cmd [list report_checks \
  -from {riscv.dp.pcreg.q[31]$_DFF_PP0_/CLK} \
  -through _10996_/Y \
  -through _14612_/Y \
  -through dmem/_156_/Y \
  -to {riscv.dp.rf.rf[13][17]$_DFFE_PP_/D}]

estimate_parasitics -placement
{*}$report_checks_cmd

remove_buffers

# 3 report_checks results should be the same.
{*}$report_checks_cmd

estimate_parasitics -placement
{*}$report_checks_cmd

sta::network_changed
estimate_parasitics -placement
{*}$report_checks_cmd

set verilog_out [make_result_file $test_name.v]
write_verilog $verilog_out
diff_file $test_name.vok $verilog_out

include "helpers.tcl"
read_liberty sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_verilog ibex_sky130hd.v
link_design ibex_core
create_clock [get_ports clk_i] -name core_clock -period 10
clock_gating -max_cover 50
set verilog_file [make_result_file ibex_sky130hd_gated_tcl.v]
write_verilog $verilog_file
diff_file ibex_sky130hd_gated.vok $verilog_file

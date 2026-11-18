# scan_opt on a chain that mixes falling- and rising-edge cells, after the
# cells moved since execute_dft_plan.
source "helpers.tcl"
source "scan_opt_helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_architect_sky130.v
link_design scan_architect

create_clock -name clock1 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock1}]
create_clock -name clock2 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock2}]

set_dft_config -max_length 20 -clock_mixing clock_mix

scan_replace
scatter_insts 0
execute_dft_plan

set lasts [scan_chain_last_cells]
scatter_insts 11
scan_opt

check_scan_chains $lasts

set verilog_file [make_result_file scan_opt_edges_sky130.v]
write_verilog $verilog_file
diff_files $verilog_file scan_opt_edges_sky130.vok

set def_file [make_result_file scan_opt_edges_sky130.def]
write_def $def_file
diff_files $def_file scan_opt_edges_sky130.defok

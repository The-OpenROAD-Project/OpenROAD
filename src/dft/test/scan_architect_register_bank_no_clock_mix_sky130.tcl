source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog register_bank_top.v
link_design top 

create_clock -name clk1 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk1}]
create_clock -name clk2 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clk2}]

set_dft_config -max_length 20000 -clock_mixing no_mix

scan_replace
preview_dft -verbose
insert_dft

set verilog_file [make_result_file scan_architect_register_bank_no_clock_mix_sky130.v]
write_verilog $verilog_file
diff_files $verilog_file scan_architect_register_bank_no_clock_mix_sky130.vok

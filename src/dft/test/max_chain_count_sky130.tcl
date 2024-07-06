source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog max_chain_count_sky130.v
link_design max_chain_count

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

set_dft_config -max_length 4 -max_chains 1

scan_replace

preview_dft -verbose
insert_dft

set verilog_file [make_result_file max_chain_count_sky130.v]
write_verilog $verilog_file
diff_files $verilog_file max_chain_count_sky130.vok

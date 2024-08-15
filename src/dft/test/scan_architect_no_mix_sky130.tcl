source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog scan_architect_sky130.v
link_design scan_architect

create_clock -name clock1 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock1}]
create_clock -name clock2 -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock2}]

set_dft_config -max_length 5

scan_replace

set verilog_file_before_preview [make_result_file scan_architect_no_mix_sky130_before_preview.v]
write_verilog -sort $verilog_file_before_preview

preview_dft -verbose

# Preview should not modify the net
set verilog_file_after_preview [make_result_file scan_architect_no_mix_sky130_after_preview.v]
write_verilog -sort $verilog_file_after_preview

# Before and after preview, the netlist should be the same
diff_files $verilog_file_after_preview $verilog_file_before_preview

insert_dft

set verilog_file [make_result_file scan_architect_no_mix_sky130.v]
write_verilog $verilog_file
diff_files $verilog_file scan_architect_no_mix_sky130.vok

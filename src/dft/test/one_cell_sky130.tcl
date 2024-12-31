source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_def one_cell_sky130.def

create_clock -name main_clock -period 2.0000 -waveform {0.0000 1.0000} [get_ports {clock}]

set_dft_config -max_length 10

report_instance ff1
scan_replace
report_instance ff1
preview_dft -verbose
report_instance ff1
insert_dft
report_instance ff1

set verilog_file [make_result_file one_cell_sky130.v]
write_verilog $verilog_file
diff_files $verilog_file one_cell_sky130.vok

set def_file [make_result_file one_cell_sky130.def]
write_def $def_file
diff_files $def_file one_cell_sky130.defok

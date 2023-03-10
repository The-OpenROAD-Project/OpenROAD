source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog sub_modules_sky130.v
link_design sub_modules

insert_dft

set verilog_file [make_result_file sub_modules_sky130.v]
write_verilog $verilog_file
diff_files $verilog_file sub_modules_sky130.vok

# make_result_file, diff_files
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

read_verilog eliminate_dead_logic1.v
link_design top

set_dont_touch \q2[0]
eliminate_dead_logic

set verilog_file [make_result_file eliminate_dead_logic1.v]
write_verilog $verilog_file
diff_files $verilog_file eliminate_dead_logic1.vok

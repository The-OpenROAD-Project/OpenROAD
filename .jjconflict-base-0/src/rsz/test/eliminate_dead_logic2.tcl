# make_result_file, diff_files
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib

read_verilog eliminate_dead_logic1.v
link_design top

add_global_connection -net {VDD} -pin_pattern {VPWR} -power
add_global_connection -net {VSS} -pin_pattern {VGND} -ground

set_dont_touch \q2[0]
eliminate_dead_logic

set verilog_file [make_result_file eliminate_dead_logic2.vg]
write_verilog -include_pwr_gnd $verilog_file
diff_files $verilog_file eliminate_dead_logic2.vgok

# Forward retiming of an enable flop whose enable is a primary input
# (sky130hd): it slides as an enable register and keeps its cell and enable
# net, with nothing unmapped.
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_unmap.v
link_design top
create_clock -period 0.5 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

set v_file [make_result_file retime_unmap.v]
write_verilog $v_file
diff_files retime_unmap.vok $v_file

# A register with an input left unconnected never moves (sky130hd).  r3 and
# r4 are the same enable cell with the enable pin DE open on both: they
# would move across g20 as one class and the new register would have no
# enable net to connect.  Both stay, g20 is never crossed, and the plain
# cone is retimed.  The floating DE makes sequential equivalence
# meaningless, so the netlist is compared with a golden one.
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_open_input.v
link_design top
create_clock -period 0.5 clk

report_worst_slack -max -digits 4
report_tns -digits 4
retime
report_worst_slack -max -digits 4
report_tns -digits 4

set v_file [make_result_file retime_open_input.v]
write_verilog $v_file
diff_files retime_open_input.vok $v_file

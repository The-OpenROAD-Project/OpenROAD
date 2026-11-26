# An isolation cell is not a plain gate, though its liberty function is one:
# no register crosses it (sky130hd).
source "helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_isolation.v
link_design top
create_clock -period 0.6 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set data [[[ord::get_db_block] findInst iso] findITerm A]
puts "iso/A reads [[$data getNet] getName]"

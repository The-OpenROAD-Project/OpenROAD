# A cell that is not a plain combinational gate is never crossed: the
# registers in front of a tri-state driver stay, on a failing path too.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_tristate.v
link_design top
create_clock -period 0.2 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set i1_input [[[ord::get_db_block] findInst i1] findITerm A]
puts "i1/A reads [[$i1_input getNet] getName]"

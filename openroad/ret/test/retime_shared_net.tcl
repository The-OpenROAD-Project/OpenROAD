# A net with two drivers.  A register that crossed one of them would delay
# what the other drives, so neither driver is crossed and nothing moves.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_shared_net.v
link_design top
create_clock -period 0.1 clk

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set i1_input [[[ord::get_db_block] findInst i1] findITerm A]
puts "i1/A reads [[$i1_input getNet] getName]"

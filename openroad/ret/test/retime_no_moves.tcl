# No legal forward move on a failing path: retime reports that it found no
# slides and changes nothing.  The output delay makes the path from r1
# through g1 to the port y fail, and g1, which drives that port, is never
# crossed: both gates of the graph are pinned.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_no_moves.v
link_design top
create_clock -period 0.1 clk
set_output_delay 0.05 -clock clk [get_ports y]

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4
set block [ord::get_db_block]
puts "r1: [expr { [$block findInst r1] == "NULL" ? "removed" : "stays" }]"
puts "g1/A reads [[[[$block findInst g1] findITerm A] getNet] getName]"

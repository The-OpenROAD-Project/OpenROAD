# The net d1 feeds the register r1 and, through it, rD, so the two are
# rebuilt together or not at all.  With rD fixed in place r1 stays too,
# though nothing fences it: the inverter h1 after r1 is pinned (8 pinned
# gates, not 7, and 0 fenced registers) and still reads q1, while the
# independent cone of r4 retimes.  Once rD is free the second retime
# rebuilds both (7 pinned gates), and the register in front of h1 crosses
# h1 to h5 (5 slides): h1 then reads d1.  The result is proven sequentially
# equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_fixed_chain.v
link_design top
create_clock -period 0.15 clk
set block [ord::get_db_block]
set rD [$block findInst rD]
$rD setPlacementStatus FIRM

write_verilog_for_dsec retime_dsec_fixed_chain before
report_worst_slack -max -digits 4
retime
report_kept {r1 rD r4}
puts "h1/A reads [[[$block findITerm h1/A] getNet] getName]"

$rD setPlacementStatus NONE
retime
report_kept {r1 rD r4}
puts "h1/A reads [[[$block findITerm h1/A] getNet] getName]"
report_worst_slack -max -digits 4
run_dsec_test retime_dsec_fixed_chain \
  -liberty_files Nangate45/Nangate45_typ.lib -clock clk -resetn rst_n

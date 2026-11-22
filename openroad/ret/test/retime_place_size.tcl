# The cell choice of the STA check (place_size): the new register drives a
# long, heavy wire, so STA prefers a stronger cell than the base flip-flop
# for it.  Proven sequentially equivalent with ABC dsec when
# EQUIVALENCE_CHECK is set.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_plain.v
link_design top
initialize_floorplan -die_area "0 0 300 20" \
  -core_area "0 0 300 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.6 clk

# Placed in one row, in path order, 20 microns apart.
set x 5
foreach name {g0 g1 r1 r2 g2 g3 g4 g5 g6 g7 g8 r3 g9} {
  set inst [[ord::get_db_block] findInst $name]
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu 2.8]
  $inst setPlacementStatus PLACED
  incr x 20
}
set_wire_rc -resistance 1.0e-03 -capacitance 3.0
estimate_parasitics -placement

write_verilog_for_dsec retime_place_size before
report_worst_slack -max -digits 4

set_debug_level RET slack_search 1
set_debug_level RET sta_check 1
set_debug_level RET place_size 1
retime

estimate_parasitics -placement
report_worst_slack -max -digits 4
foreach inst [[ord::get_db_block] getInsts] {
  if { [string match "ret_ff*" [$inst getName]] } {
    puts "[$inst getName] is a [[$inst getMaster] getName]"
  }
}
run_dsec_test retime_place_size \
  -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

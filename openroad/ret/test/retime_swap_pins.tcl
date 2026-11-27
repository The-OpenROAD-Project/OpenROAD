# The cell choice of the STA check only tries a cell with the pin names of
# the register it replaces.  QDFF_X8 is DFF_X1 in liberty and faster, but its
# LEF has one more input: the database refuses the swap after STA has dropped
# the timing arcs of the register, which then looks as if it met timing.  The
# design is that of retime_place_size, and so is the result.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_swap_pins.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_swap_pins.lef
read_verilog retime_dsec_plain.v
link_design top
initialize_floorplan -die_area "0 0 300 20" \
  -core_area "0 0 300 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.6 clk

set x 5
foreach name {g0 g1 r1 r2 g2 g3 g4 g5 g6 g7 g8 r3 g9} {
  set inst [[ord::get_db_block] findInst $name]
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu 2.8]
  $inst setPlacementStatus PLACED
  incr x 20
}
set_wire_rc -resistance 1.0e-03 -capacitance 3.0
estimate_parasitics -placement

report_worst_slack -max -digits 4
retime
estimate_parasitics -placement
report_worst_slack -max -digits 4
foreach inst [[ord::get_db_block] getInsts] {
  if { [string match "ret_ff*" [$inst getName]] } {
    puts "[$inst getName] is a [[$inst getMaster] getName]"
  }
}

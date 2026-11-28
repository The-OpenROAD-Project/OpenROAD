# The search, the STA check and the cell choice on a placed design that fails
# timing: the new register is placed, and the result is kept because STA calls
# it better.  Proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dsec_plain.v
link_design top
initialize_floorplan -die_area "0 0 100 20" \
  -core_area "0 0 100 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.15 clk

# Placed in one row, in path order, 5 microns apart.
set x 5
foreach name {g0 g1 r1 r2 g2 g3 g4 g5 g6 g7 g8 r3 g9} {
  place_inst -name $name -location "$x 2.8"
  incr x 5
}
set_wire_rc -resistance 1.0e-03 -capacitance 1.0e-05
estimate_parasitics -placement

write_verilog_for_dsec retime_slack_search before
report_worst_slack -max -digits 4
report_tns -digits 4

retime

estimate_parasitics -placement
report_worst_slack -max -digits 4
report_tns -digits 4
report_unplaced
foreach inst [[ord::get_db_block] getInsts] {
  if { [string match "ret_ff*" [$inst getName]] } {
    lassign [$inst getLocation] x y
    puts "[$inst getName] at [ord::dbu_to_microns $x] [ord::dbu_to_microns $y]"
  }
}
run_dsec_test retime_slack_search \
  -liberty_files Nangate45/Nangate45_typ.lib \
  -clock clk -resetn rst_n

report_new_cells

# A library without a plain flip-flop (ihp-sg13g2): the base register is the
# reset flip-flop with its reset tied inactive.  On the placed design the new
# register is placed, and its own tie cell, whose only neighbor is that
# register, is placed after it.  The result is proven sequentially equivalent
# with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_verilog retime_dsec_tied_base.v
link_design top
initialize_floorplan -die_area "0 0 100 20" \
  -core_area "0 0 100 20" \
  -site CoreSite
create_clock -period 0.4 clk

# Placed in one row, in path order, 5 microns apart.
set x 5
foreach name {g0 g1 t1 t2 r1 r2 g2 g3 g4 g5 g6 g7 g8 t3 r3 g9} {
  set inst [[ord::get_db_block] findInst $name]
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu 3.78]
  $inst setPlacementStatus PLACED
  incr x 5
}
set_wire_rc -resistance 1.0e-03 -capacitance 1.0e-05
estimate_parasitics -placement

write_verilog_for_dsec retime_dsec_tied_base before
report_worst_slack -max -digits 4
retime
estimate_parasitics -placement
report_worst_slack -max -digits 4
set unplaced 0
foreach inst [[ord::get_db_block] getInsts] {
  if { ![$inst isPlaced] } {
    incr unplaced
  }
}
puts "unplaced instances: $unplaced"
foreach inst [[ord::get_db_block] getInsts] {
  if { [string match "ret_*" [$inst getName]] } {
    lassign [$inst getLocation] x y
    puts "[$inst getName] at [ord::dbu_to_microns $x] [ord::dbu_to_microns $y]"
  }
}
run_dsec_test retime_dsec_tied_base \
  -liberty_files ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_tied_base.v]
write_verilog $v_file
diff_files retime_dsec_tied_base.vok $v_file

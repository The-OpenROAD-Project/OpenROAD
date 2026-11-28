# A library without a plain flip-flop (ihp-sg13g2): the base register is the
# reset flip-flop with its reset tied inactive.  The tie cells are marked
# set_dont_use, as ORFS marks them on gt2n, and the new register still gets
# a tie cell of its own.  On the placed design the new register is placed,
# and its tie cell, whose only neighbor is that register, is placed after
# it.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
source "ret_report.tcl"
read_liberty ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib
read_lef ihp-sg13g2/sg13g2_tech.lef
read_lef ihp-sg13g2/sg13g2_stdcell.lef
read_verilog retime_dsec_tied_base.v
link_design top
initialize_floorplan -die_area "0 0 100 20" \
  -core_area "0 0 100 20" \
  -site CoreSite
create_clock -period 0.4 clk
set_dont_use {sg13g2_tiehi sg13g2_tielo}

# Placed in one row, in path order, 5 microns apart.
set x 5
foreach name {g0 g1 t1 t2 r1 r2 g2 g3 g4 g5 g6 g7 g8 t3 r3 g9} {
  place_inst -name $name -location "$x 3.78"
  incr x 5
}
set_wire_rc -resistance 1.0e-03 -capacitance 1.0e-05
estimate_parasitics -placement

write_verilog_for_dsec retime_dsec_tied_base before
report_worst_slack -max -digits 4
retime
estimate_parasitics -placement
report_worst_slack -max -digits 4
report_unplaced
foreach inst [[ord::get_db_block] getInsts] {
  if { [string match "ret_*" [$inst getName]] } {
    lassign [$inst getLocation] x y
    puts "[$inst getName] at [ord::dbu_to_microns $x] [ord::dbu_to_microns $y]"
  }
}
run_dsec_test retime_dsec_tied_base \
  -liberty_files ihp-sg13g2/sg13g2_stdcell_typ_1p20V_25C.lib \
  -clock clk -resetn rst_n

report_new_cells

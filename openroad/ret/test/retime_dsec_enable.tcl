# Enable registers: a same-class merge keeps the enable cell, a mixed-class
# merge unmaps.  On the placed design every new register and unmap gate is
# placed.  The result is proven sequentially equivalent with ABC dsec.
source "helpers.tcl"
source "ret_helpers.tcl"
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_liberty sky130hd/sky130hd_tt.lib
read_verilog retime_dsec_enable.v
link_design top
initialize_floorplan -die_area "0 0 200 20" \
  -core_area "0 0 200 20" \
  -site unithd
create_clock -period 0.75 clk

# Placed in one row, in path order, 6 microns apart.
set x 5
foreach name {g_r g_e1 g_e2 g0 g1 g1b g1c r1 r2 r4 r6 g2 g3 g4 g5 g6 g7 g8 r3 \
                g9 g10 g11 g12 g13 g14 g15 g16 r5 g17} {
  set inst [[ord::get_db_block] findInst $name]
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu 2.72]
  $inst setPlacementStatus PLACED
  incr x 6
}
set_wire_rc -resistance 1.0e-03 -capacitance 1.0e-05
estimate_parasitics -placement

write_verilog_for_dsec retime_dsec_enable before
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
run_dsec_test retime_dsec_enable -liberty_files sky130hd/sky130hd_tt.lib \
  -clock clk -resetn rst_n

set v_file [make_result_file retime_dsec_enable.v]
write_verilog $v_file
diff_files retime_dsec_enable.vok $v_file

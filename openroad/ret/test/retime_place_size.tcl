# The cell choice of the STA check: the new register drives a long, heavy wire,
# so STA prefers a stronger cell than the base flip-flop for it.  It only
# tries cells with the pin names of the register it replaces: QDFF_X8 is
# DFF_X1 in liberty and faster, but its LEF has one more input, and the
# database would refuse the swap after STA had dropped the timing arcs of the
# register, which would then look as if it met timing.  Nor does it take a
# cell whose timing arcs are not the register's: DFF_NOARC is DFF_X1 without
# arcs, and the paths through it would look as if they met timing.  The
# result is proven sequentially equivalent with ABC dsec.
# top_pins: a register of QDFF_X8 never moves either, as the LEF pins of its
# cell are not its liberty pins; the same cone with a DFF_X1 retimes.
source "helpers.tcl"
source "ret_helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_liberty retime_place_size.lib
read_lef Nangate45/Nangate45.lef
read_lef retime_place_size.lef
read_verilog retime_dsec_plain.v
link_design top
initialize_floorplan -die_area "0 0 300 20" \
  -core_area "0 0 300 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.6 clk

# Placed in one row, in path order, 20 microns apart.
set x 5
foreach name {g0 g1 r1 r2 g2 g3 g4 g5 g6 g7 g8 r3 g9} {
  place_inst -name $name -location "$x 2.8"
  incr x 20
}
set_wire_rc -resistance 1.0e-03 -capacitance 3.0
estimate_parasitics -placement

write_verilog_for_dsec retime_place_size before
report_worst_slack -max -digits 4

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

read_verilog retime_place_size.v
link_design top_pins
set block [ord::get_db_block]
# read_verilog drops EXTRA, which the liberty cell lacks; an open input would
# keep rq in place by itself.
set extra [[$block findInst rq] findITerm EXTRA]
$extra connect [$block findNet x]
puts "rq EXTRA reads [[$extra getNet] getName]"
create_clock -period 0.1 clk
# The parasitics estimated for the first design are stale here.
estimate_parasitics -placement

retime
foreach name {r1 rq} {
  set inst [$block findInst $name]
  if { $inst == "NULL" } {
    puts "$name: moved"
  } else {
    puts "$name: stays, a [[$inst getMaster] getName]"
  }
}
foreach name {g1 g7} {
  set net [[[$block findInst $name] findITerm A] getNet]
  puts "$name reads [$net getName]"
}

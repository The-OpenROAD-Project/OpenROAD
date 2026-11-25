# r1 sits at the end of a long wire from g0; moving it across h1 puts that
# wire on the path from r0.  With a resistive wire the search proposes the
# move and the STA check rejects it, so nothing moves; with a fast wire r1
# moves.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_wire.v
link_design top
initialize_floorplan -die_area "0 0 1000 20" \
  -core_area "0 0 1000 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 0.14 clk

# Placed locations in microns.
proc place_inst { name x } {
  set inst [[ord::get_db_block] findInst $name]
  $inst setLocation [ord::microns_to_dbu $x] [ord::microns_to_dbu 2.8]
  $inst setPlacementStatus PLACED
}
set x 10
foreach name {gi r0 g0} {
  place_inst $name $x
  incr x 5
}
set x 900
foreach name {r1 h1 h2 h3 h4 r2 go} {
  place_inst $name $x
  incr x 5
}

set_wire_rc -resistance 1.0e-01 -capacitance 1.0e-05
estimate_parasitics -placement
retime

set_wire_rc -resistance 1.0e-05 -capacitance 1.0e-05
estimate_parasitics -placement
retime

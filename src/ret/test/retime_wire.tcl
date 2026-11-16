# The delay of an edge that loses its register includes the wire into that
# register.  r1 sits at the end of a long wire from g0; moving it across h1
# would make that wire part of a register-to-register path, so with a
# resistive wire nothing moves, and with a fast wire r1 does.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_wire.v
link_design top
initialize_floorplan -die_area "0 0 1000 20" \
  -core_area "0 0 1000 20" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
create_clock -period 1.0 clk

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

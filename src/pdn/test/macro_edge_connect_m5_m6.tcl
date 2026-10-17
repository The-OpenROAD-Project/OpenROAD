# Two edge connections crossing each other take a via.
#
# The same design as macro_edge_connect, with {metal5 metal6} added to the
# macro grid so that the two layers its pins are on can reach each other.
#
# On a rectangle that changes nothing: its stubs are on four walls facing four
# ways, so the wires grown out of them lead away from the macro and from one
# another.  Inside the notch of a polygon macro they do meet.  macro_U carries
# a metal5 stub on the inner wall of its left leg and a metal6 stub on the
# floor of the bend, and both grow into the bend: the metal5 one east from
# (98 88)-(102 88.48) to the core metal4 strap at x = 112.87, the metal6 one
# north from (106 81.4)-(106.48 85.4) to the core metal7 strap at y = 94.1.
# They cross at x = 106, y = 88, and with a rule between the two layers the
# grid puts a via there -- (106.24 88.24) in the golden, which is the one via
# this test has that macro_edge_connect does not.
#
# Everything else is as it was.  The rule gives every metal5 pin metal6 to
# look at and every metal6 pin metal5, but what this pass builds is never a
# target for it, so no pin is grown to another pin's way out of the macro and
# all 33 connections are the ones macro_edge_connect makes.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/edge_pin_macros.lef
read_def nangate_polygon/floorplan_edge_macros.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -spacing 0.56 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -spacing 1.60 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "edge_pins" \
  -instances {macro_rect macro_L macro_T macro_U}
add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}

pdngen

set def_file [make_result_file macro_edge_connect_m5_m6.def]
write_def $def_file
diff_files macro_edge_connect_m5_m6.defok $def_file

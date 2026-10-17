# An edge connection whose via cannot be built is withdrawn.
#
# The same macro as macro_edge_connect_blocked, on a grid whose one connect
# rule is {metal5 metal8}.  metal8 is three layers up, so a via out of a metal5
# pin is a stack through metal6 and metal7, and an obstruction on either of
# those rejects it.
#
# The test puts one on metal6 across (73 41)-(76 45), which is where the stack
# for the VSS stub on the east wall would land.  That stub is grown and kept --
# it survives cutting, since the obstruction is on a layer it does not use, and
# it reaches the metal8 strap at x = 73.59 -- and then no via is built on it.
#
# Nothing downstream would take it back.  The landing on the pin counts as a
# connection, so the shape is not floating and trimming keeps it, and what
# would be written is metal5 running 15um out of the macro to nothing.  The
# pass withdraws it instead, so the golden holds no metal5 wire at all -- its
# only metal5 is the 21 patches the core's own metal4 to metal7 stacks leave
# on the way past.
#
# The other three pins are the cases macro_edge_connect_blocked already covers,
# in the arrangement this grid gives them:
#
#   VDD  metal5 (43 28.8)-(47 29.28)   walled in by the macro's own metal5 at
#                                      36 <= x <= 42 and 48 <= x <= 54, both
#                                      ends tried, neither kept.
#   VSS  metal5 (30 38.8)-(34 39.28)   already connected -- a metal8 strap
#                                      crosses it, which is the ordinary
#                                      top-down connection, so nothing is
#                                      attempted.
#   VDD  metal6 (56 45)-(56.48 49)     no rule names metal6.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/edge_pin_macros.lef
read_def nangate_polygon/floorplan_blocked_macro.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

# in the way of the via stack, not of the wire
set layer [[ord::get_db_tech] findLayer metal6]
odb::dbObstruction_create [ord::get_db_block] $layer \
  [ord::microns_to_dbu 73] [ord::microns_to_dbu 41] \
  [ord::microns_to_dbu 76] [ord::microns_to_dbu 45]

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -spacing 0.56 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -spacing 1.60 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal8 -width 0.96 -spacing 1.12 -pitch 20.0 -offset 4.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal7 metal8}

define_pdn_grid -macro -name "blocked" -instances {macro_blocked}
add_pdn_connect -layers {metal5 metal8}

pdngen

set def_file [make_result_file macro_edge_connect_no_via.def]
write_def $def_file
diff_files macro_edge_connect_no_via.defok $def_file

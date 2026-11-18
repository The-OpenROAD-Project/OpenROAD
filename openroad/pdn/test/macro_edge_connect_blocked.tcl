# A pin with no way out of its macro is left alone.
#
# edge_macro_blocked draws two metal5 rectangles of its own, which are the
# macro's routing and not a pin of anything.  Against its placement at
# (30 23.8) they cover 36 <= x <= 42 and 48 <= x <= 54, and the four supply
# pins are:
#
#   VDD  metal5 (43 28.8)-(47 29.28)   between the two, and the same distance
#                                      from the macro wall at either end, so
#                                      both ends are tried.  Each is cut short
#                                      of the rectangle it runs into and
#                                      reaches nothing, so neither is kept.
#   VSS  metal5 (30 38.8)-(34 39.28)   west of both, on the macro wall, and
#                                      the one pin here that connects: it
#                                      grows west to the core metal4 strap at
#                                      x = 11.83.
#   VSS  metal5 (56 42.8)-(60 43.28)   east of both, on the macro wall, and
#                                      blocked from outside instead -- the
#                                      test puts a metal5 obstruction across
#                                      (60 40)-(76 45) in its way.  East is
#                                      its way out and the only end tried:
#                                      west is 26um of macro away and growing
#                                      that way would be growing into the
#                                      macro, so nothing is kept for it.
#   VDD  metal6 (56 45)-(56.48 49)     on a layer no connect rule in the grid
#                                      names.  What a pin may reach is the
#                                      connect rules and nothing else, so
#                                      nothing is attempted for this one at
#                                      all; macro_edge_connect, where metal6
#                                      pins do have a rule and do connect, is
#                                      what says the difference is the rule.
#
# As in macro_edge_connect the rows are cut to the outline of the macro plus a
# 2um margin and tap cells are inserted, so nothing stands under it.
#
# The golden holds exactly one metal5 stripe, the VSS one growing west out of
# the macro wall at x = 30.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_polygon/edge_pin_macros.lef
read_def nangate_polygon/floorplan_blocked_macro.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

# in the way of the pin on the east wall
set layer [[ord::get_db_tech] findLayer metal5]
odb::dbObstruction_create [ord::get_db_block] $layer \
  [ord::microns_to_dbu 60] [ord::microns_to_dbu 40] \
  [ord::microns_to_dbu 76] [ord::microns_to_dbu 45]

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -spacing 0.56 -pitch 20.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.40 -spacing 1.60 -pitch 20.0 -offset 2.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "blocked" -instances {macro_blocked}
add_pdn_connect -layers {metal4 metal5}

pdngen

set def_file [make_result_file macro_edge_connect_blocked.def]
write_def $def_file
diff_files macro_edge_connect_blocked.defok $def_file

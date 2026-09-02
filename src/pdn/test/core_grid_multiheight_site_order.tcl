# Reproducer for a multi-height row failure that shows up when the taller
# sites' rows precede the base rows in the database.
#
# sky130_multiheight_rows/floorplan_multisite.def was produced by
#   initialize_floorplan -die_area {0 0 40 48} -core_area {5.52 5.44 34.04 38.08} \
#     -site core_9T -additional_sites {core_18T core_27T core_36T}
# and mirrors the reporter's platform in #10955:
#   PLACE_SITE = core_700 ; ADDITIONAL_SITES = core_1400 core_2100 core_2800
#
# InitFloorplan::makeUniformRows walks its sites through a map keyed by site
# NAME, creating every row of one site before moving to the next, so database
# order is alphabetical by name.  "core_18T", "core_27T" and "core_36T" all sort
# ahead of "core_9T" because a single digit follows the underscore -- the same
# inversion core_700 gets against core_1400/2100/2800.  The 12 base rows are
# therefore created last, after the 6 double, 4 triple and 3 quadruple rows.
#
# ifp alternates R0/MX down each site independently, which is right: for a row
# spanning an even number of base rows the two orientations describe the same
# net arrangement, so the flip is a no-op there.  But FollowPins::makeShapes
# reads power-vs-ground off each row's own orientation, and for an even multiple
# that orientation carries no phase information -- so consecutive core_18T rows,
# and consecutive core_36T rows, disagree about which net sits on their shared
# boundary.  The straps then collide in GridComponent::addShape, which drops the
# loser with nothing but a debug message, so whichever row came first in the
# database silently wins.
#
# The rails must alternate VSS/VDD on every 2.72um row boundary.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_multiheight_rows/multiheight_sites.lef
read_def sky130_multiheight_rows/floorplan_multisite.def

set block [ord::get_db_block]

add_global_connection -net VDD -pin_pattern {^VPWR$} -power
add_global_connection -net VSS -pin_pattern {^VGND$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 20.000 -offset 10.000
add_pdn_connect -layers {met1 met4}

pdngen

set def_file [make_result_file core_grid_multiheight_site_order.def]
write_def $def_file
diff_files core_grid_multiheight_site_order.defok $def_file

# Power switch insertion on a polygon floorplan.
#
# sky130_power_switch/floorplan_polygon.def is the gcd design of floorplan.def
# on an L-shaped die and core:
#
#   die   (0 0) (276 0) (276 136) (138 136) (138 277.44) (0 277.44)
#   core  (10.12 10.88) (265.88 10.88) (265.88 130.56) (132.48 130.56)
#         (132.48 266.56) (10.12 266.56)
#
# The inset is 10.12um and 10.88um at the outer edges but only 5.52um and
# 5.44um at the concave corner, so a shape that fits the core bounding box can
# still cross the real die boundary.  The core height is a whole number of
# double rows measured from the core origin, because the switch cell is two
# rows tall and sits in the unithddbl row set: 94 unithd rows and 47 unithddbl
# rows, the latter 22 across the base and 25 up the arm.
#
# Power switch insertion reads rows and straps, and both of those follow the
# outline by the time it runs, so it is expected to work here without knowing
# anything about polygons.  This test is what says so.  Measured: 298 switches,
# in all 47 double rows, 202 across the base and 96 up the arm.  The arm is
# 122.36um wide against the 255.76um of the base, and the switches respect it --
# the rightmost in the arm ends at x = 115.00 against 250.70 in the base -- as
# do the straps that carry them: the met4 straps at x > 132.48 stop at the
# notch floor and the met5 straps above it stop at the arm.  None of the 298
# lands in the die notch, and PDN-0223, which withdraws a switch that did not
# find two rows to sit in, does not fire.
#
# One bounding box does survive in this path.  GridSwitchedPower::build hands
# getCoreArea() to computeLocations, which uses only its xMin() as the origin
# to measure site widths from.  Both arms of a core start on the same site
# grid, so that is the same value the rows themselves are built from; it would
# take a floorplan whose rows do not share an origin to make it wrong, which
# initialize_floorplan cannot produce.
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_power_switch/power_switch.lef

read_def sky130_power_switch/floorplan_polygon.def

add_global_connection -net VDD -power -pin_pattern "^VDDG$"
add_global_connection -net VDD_SW -power -pin_pattern "^VPB$"
add_global_connection -net VDD_SW -pin_pattern "^VPWR$"
add_global_connection -net VSS -power -pin_pattern "^VGND$"
add_global_connection -net VSS -power -pin_pattern "^VNB$"

set_voltage_domain -power VDD -ground VSS -switched_power VDD_SW
define_power_switch_cell -name POWER_SWITCH -control SLEEP -acknowledge SLEEP_OUT \
  -power_switchable VPWR -power VDDG -ground VGND
define_pdn_grid -name "Core" -power_switch_cell POWER_SWITCH -power_control nPWRUP \
  -power_control_network DAISY

add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 27.140 -offset 13.570
add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600
add_pdn_connect -layers {met1 met4}
add_pdn_connect -layers {met4 met5}

pdngen

set def_file [make_result_file polygon_power_switch.def]
write_def $def_file
diff_files polygon_power_switch.defok $def_file

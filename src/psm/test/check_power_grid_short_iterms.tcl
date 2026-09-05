# Check that check_power_grid reports a power stripe running over the
# signal pins (dbITerm) of the cells it crosses, the failure mode from
# The-OpenROAD-Project/OpenROAD#10955.  The two metal1 VDD stripes cover
# the A and ZN pins of u1/u2/u5/u6; the pins of u3/u4/u7/u8 are controls
# and must never be reported.
#
# VDD is strapped from metal1 up to metal3 through via1/via2 and topped
# with a 45 degree octagonal bump, so the net being walked carries wires,
# vias and a non-Manhattan shape.  odb decomposes the LEF polygon on a
# rectilinear path, so the bump's dbBox geometry only approximates the
# octagon that dbPolygon::getPolygon() holds.
#
# The stripes cross the VSS pin of each cell as well as A and ZN, so the
# golden holds three violations per cell rather than two.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_lef short_pads.lef
read_def check_power_grid_short_iterms.def

catch { check_power_grid -net VDD -dont_require_terminals } err
puts $err

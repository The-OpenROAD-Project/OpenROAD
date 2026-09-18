# Check that check_power_grid reports the power net shorting to objects
# that are not part of it: fill (dbFill), a block terminal of another net
# (dbBTerm), the special wiring of another net and the via on it
# (dbSWire), signal routing and the via on it (dbWire), and a 45 degree
# octagonal pad on a signal net (dbITerm).  VDD itself is strapped from
# metal1 up to metal3 through via1/via2, so the net being walked carries
# vias as well as wires.
#
# The metal2 fill and the objects placed clear of the VDD rail are
# controls and must never be reported.
#
# The x range of the in2 route in the golden is wider than the one listed
# above: the coordinates above are the path ends, the shape adds the half
# width end extension to each of them.  The VSS stripe and the via on it
# overlap the rail over the same area, so they are two violations with the
# same coordinates rather than one.
source helpers.tcl

read_lef Nangate45/Nangate45.lef
read_lef short_pads.lef
read_def check_power_grid_short.def

catch { check_power_grid -net VDD -dont_require_terminals } err
puts $err

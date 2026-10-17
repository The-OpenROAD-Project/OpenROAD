# A ring that fits the die bounding box but leaves the polygon die.
#
# This is the case PDN-0351 was added for and does not currently catch.  On the
# L-shaped floorplan of nangate_polygon/floorplan.def the core-to-die margin is
# 9.5um in X and 8.4um in Y at the outer edges, but only 3.8um/2.8um at the
# concave corner.  A 2.0um offset plus 6.0um of metal is 8.0um, so:
#
#   * against the die *bounding box* (0 0) - (95 112) the ring fits, which is
#     all Rings::checkDieArea tests today, so it reports success;
#   * against the real die outline the ring crosses the notch boundary by
#     4.2um in X and 5.2um in Y, and the system obstructions over the notch
#     then break the ring into disconnected segments with no diagnostic.
#
# PDN-0351 must fire here.
#
# The 4.2 and 5.2 in the message are the deficits at the notch: the ring needs
# 8.0um and has 3.8um in X and 2.8um there.  They are measured per side of the
# core rather than from the area that overruns, which on a polygon die would
# report the length of the offending side instead of how far it went past.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

catch {
  add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 2.0 \
    -spacings 2.0 -core_offsets 2.0 -add_connect
} err
puts $err

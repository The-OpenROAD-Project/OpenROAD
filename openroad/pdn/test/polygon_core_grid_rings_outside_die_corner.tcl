# A ring that leaves the die only where it turns a corner.
#
# nangate_polygon/floorplan_cut_corner_die.def is the 133x112 die of the macro
# fixtures with one corner cut away, past the end of both core edges:
#
#   die   (0 0) (133 0) (133 103.6) (123.5 103.6) (123.5 112) (0 112)
#   core  (9.5 8.4) - (123.5 103.6), a rectangle
#
# Every side of the core keeps its full room -- 9.5um to the right of it and
# 8.4um above it -- and the ring needs only 2.2um, so no side is short.  What
# does not fit is the square where the two sides meet, (123.5 103.6) to
# (125.7 105.8), which is exactly the piece the die is missing.
#
# The check is an area containment and catches it.  The message did not:
# Rings::getDieAreaDeficit walks the edges of the domain and asks how far each
# one may grow, and this cut lies beyond the end of both, so both deficits came
# out zero and the error read "by 0 um in X and 0 um in Y".  It now falls back
# to the size of the part that is actually outside the die, and says 2.2 and
# 2.2 -- the corner square, whole.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_polygon/floorplan_cut_corner_die.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

catch {
  add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 0.6 \
    -spacings 0.4 -core_offsets 0.6 -add_connect
} err
puts $err

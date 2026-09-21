# Test for -min_width_layers on add_pdn_connect.
#
# metal1 (followpins) connects up to a wide metal4 strap through the pass-through
# routing layers metal2 and metal3.  By default the via array fills the strap
# overlap, so the metal2 enclosure grows very wide (~1.88um here) and would block
# adjacent routing tracks.  Listing those layers in -min_width_layers forces the
# array to a single row/column in the layer's width direction, keeping the
# pass-through metal at its minimum landing width.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring
add_pdn_stripe -layer metal4 -width 1.0 -pitch 5.0 -offset 2.5 -extend_to_core_ring -snap_to_grid

add_pdn_connect -layers {metal1 metal4} -min_width_layers {metal2 metal3}

pdngen

# Report the widest enclosure on each pass-through layer (in its non-preferred,
# width direction) across the generated power vias.  With -min_width_layers these
# stay at the minimal single-cut landing width.
proc report_passthrough_width { layer_name } {
  set block [ord::get_db_block]
  set dbu [$block getDbUnitsPerMicron]
  set layer [[ord::get_db_tech] findLayer $layer_name]
  set horizontal [expr { [$layer getDirection] == "HORIZONTAL" }]

  set max_width 0
  foreach via [$block getVias] {
    foreach box [$via getBoxes] {
      if { [[$box getTechLayer] getName] ne $layer_name } {
        continue
      }
      set width [expr {
        $horizontal ? [$box yMax] - [$box yMin]
        : [$box xMax] - [$box xMin]
      }]
      if { $width > $max_width } {
        set max_width $width
      }
    }
  }

  set min_width [$layer getWidth]
  puts "$layer_name: max enclosure width\
 [format %.3f [expr { $max_width / double($dbu) }]]um\
 (single-cut landing <= [format %.3f [expr { 2.0 * $min_width / double($dbu) }]]um):\
 [expr { $max_width <= 2 * $min_width ? "PASS" : "FAIL" }]"
}

report_passthrough_width metal2
report_passthrough_width metal3

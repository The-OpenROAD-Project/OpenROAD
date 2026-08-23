source "helpers.tcl"
read_lef "Nangate45/Nangate45_tech.lef"
read_lef "macro_pin_escape.lef"
read_def "macro_pin_escape.def"

# With one required AP, the first on-grid candidate is close enough to the
# macro boundary that normal candidate generation would stop before creating
# the centered candidate.
pin_access -min_access_points 1 -verbose 0

set db [ord::get_db]
set block [[$db getChip] getBlock]
set macro [$block findInst macro]
set macro_bbox [[$macro getMaster] getPlacementBoundary]
set metal2 [[$db getTech] findLayer metal2]
set edge_margin [expr { 2 * [$metal2 getWidth] }]

set macro_ap_count 0
set far_ap_count 0
foreach ap [$block getAccessPoints] {
  if { [$ap getMPin] eq "NULL" || [[$ap getLayer] getName] ne "metal2" } {
    continue
  }

  incr macro_ap_count
  set point [$ap getPoint]
  set x [lindex $point 0]
  set y [lindex $point 1]
  if {
    $x >= [$macro_bbox xMin] + $edge_margin
    && $x <= [$macro_bbox xMax] - $edge_margin
    && $y >= [$macro_bbox yMin] + $edge_margin
    && $y <= [$macro_bbox yMax] - $edge_margin
  } {
    incr far_ap_count
  }
}

check "macro APs were generated" {expr { $macro_ap_count > 0 }} 1
check "a macro AP is far from every instance edge" {expr { $far_ap_count > 0 }} 1
exit_summary

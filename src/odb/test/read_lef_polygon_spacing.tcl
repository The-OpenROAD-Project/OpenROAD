# OBS POLYGON geometry keeps the LEF SPACING modifier
source "helpers.tcl"

set db [ord::get_db]
read_lef "sky130hd/sky130hd.tlef"
read_lef "read_lef_polygon_spacing.lef"

set master [$db findMaster poly_obs]

foreach poly [$master getPolygonObstructions] {
  set layer [[$poly getTechLayer] getName]
  set spacing($layer) [$poly getMinSpacing]
  set box_spacing($layer) {}
  foreach box [$poly getGeometry] {
    lappend box_spacing($layer) [$box getMinSpacing]
  }
}

check "met1 polygon spacing" { set spacing(met1) } 200
check "met1 box count" { llength $box_spacing(met1) } 2
check "met1 box spacing" { lsort -unique $box_spacing(met1) } 200
check "met2 polygon spacing" { set spacing(met2) } -1
check "met2 box count" { llength $box_spacing(met2) } 2
check "met2 box spacing" { lsort -unique $box_spacing(met2) } -1

exit_summary

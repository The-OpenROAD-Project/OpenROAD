source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef multi_height_power_offset.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog reg1.v
link_design top
initialize_floorplan \
  -die_area "0 0 19 0 19 8.4 9.5 8.4 9.5 14 0 14" \
  -core_area "0 0 19 0 19 8.4 9.5 8.4 9.5 14 0 14" \
  -site FreePDK45_38x28_10R_NP_162NW_34O \
  -additional_sites DoubleExtSite

set poly_rows 0
set expected_ymins {2800 8400 14000 19600}
foreach row [[ord::get_db_block] getRows] {
  if { [[$row getSite] getName] eq "DoubleExtSite" } {
    incr poly_rows
    set bbox [$row getBBox]
    set ymin_dbu [$bbox yMin]
    if { $ymin_dbu ni $expected_ymins } {
      error "Unexpected polygon DoubleExtSite row yMin=$ymin_dbu"
    }
    puts [format "Polygon DoubleExtSite row: %s y=%.3f..%.3f x=%.3f..%.3f" \
      [$row getName] \
      [ord::dbu_to_microns $ymin_dbu] \
      [ord::dbu_to_microns [$bbox yMax]] \
      [ord::dbu_to_microns [$bbox xMin]] \
      [ord::dbu_to_microns [$bbox xMax]]]
  }
}
if { $poly_rows == 0 } {
  error "Expected DoubleExtSite rows in polygon floorplan"
}

# test that -halo and ring -core_offsets are remapped for every orientation
#
# Both are given as {left bottom right top} in the macro's as-drawn frame and
# name edges, not directions, so the whole placement transform applies to them:
# a flip moves a value to the opposite edge, a quarter turn to an adjacent one
# (R90 puts the drawn left edge on the placed bottom).  This is the same
# remapping dbInst::getTransformedHalo does for a DEF halo.
#
# The values are all distinct so each edge can be told from the others, and
# "pdngen -report_only" prints the resolved values without building shapes,
# which keeps the halo off this floorplan's rows (PDN-0008).
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

set macro "dcache.data.data_arrays_0.data_arrays_0_ext.mem"

# The macro moves through every orientation below, so put the design at the
# point a real flow runs pdngen from -- macros placed, standard cells not yet
# placed -- rather than leaving it arranged around the macro as it was drawn.
# The rows are left as they are: -report_only builds no shapes, so nothing is
# ever drawn over them.
proc unplace_std_cells { } {
  foreach inst [[ord::get_db_block] getInsts] {
    if { ![[$inst getMaster] isBlock] } {
      $inst setPlacementStatus NONE
    }
  }
}

# Re-orient the macro and re-place it.  A quarter turn takes this macro from
# 38 x 112 um to 112 x 38 um, so it cannot stay where it was drawn; this puts
# it somewhere that fits inside the core in either footprint.  FIXED instances
# have to be released to be re-oriented.
proc place_rotated { name orient x y } {
  set inst [[ord::get_db_block] findInst $name]
  set status [$inst getPlacementStatus]
  $inst setPlacementStatus PLACED
  $inst setLocationOrient $orient
  $inst setLocation $x $y
  $inst setPlacementStatus $status
}

unplace_std_cells

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

foreach orient {R0 MY MX R180 R90 MXR90 MYR90 R270} {
  pdngen -reset
  place_rotated $macro $orient 49970 100800

  set_voltage_domain -power VDD -ground VSS

  define_pdn_grid -macro -name "sram" -instances $macro -halo {1 2 3 4}
  add_pdn_ring -grid "sram" -layers {metal5 metal6} \
    -widths 2.0 -spacings 2.0 -core_offsets {5 6 7 8}

  pdngen -report_only
}

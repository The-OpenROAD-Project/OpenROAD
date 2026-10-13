# test the half-turn half of the right-angle orientations
#
# macros_flipped_right_angle covers W and FW; the other two right-angle
# orientations are each of those turned 180, and a half turn reverses both
# placed axes.  Measured against R90, the frame a rotated macro's grid is
# written in:
#   - MYR90 (FE) is R90 with the placed y mirrored, so the horizontal (metal5)
#     offsets are measured from the top edge and the vertical (metal6) ones
#     stay put
#   - R270 (E) is R90 turned 180, so both mirror
# Between the two tests all four right-angle orientations, and all four
# combinations of the two mirror flags, are covered.
#
# No halo here: the core is fully rowed, so any halo intrudes into rows the
# instance does not cover (PDN-0008).  macros_flipped_right_angle_halo covers
# -halo and ring -core_offsets instead, through "pdngen -report_only".
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

# Rotating a macro invalidates everything the fixture had arranged around it:
# the rows were cut for the macros as they were drawn, and the standard cells
# sit clear of that footprint.  Put the design back at the point a real flow
# runs pdngen from -- macros placed, standard cells not yet placed -- so the
# rows can be re-cut for where the macros actually end up.
proc unplace_std_cells { } {
  foreach inst [[ord::get_db_block] getInsts] {
    if { ![[$inst getMaster] isBlock] } {
      $inst setPlacementStatus NONE
    }
  }
}

# Re-orient a macro and re-place it.  A quarter turn takes this macro from
# 38 x 112 um to 112 x 38 um, so it cannot stay where it was drawn; this puts
# it somewhere that still fits inside the core.  FIXED instances have to be
# released to be re-oriented.
proc place_rotated { name orient x y } {
  set inst [[ord::get_db_block] findInst $name]
  set status [$inst getPlacementStatus]
  $inst setPlacementStatus PLACED
  $inst setLocationOrient $orient
  $inst setLocation $x $y
  $inst setPlacementStatus $status
}

unplace_std_cells

place_rotated "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem" "MYR90" 49970 100800
place_rotated "dcache.data.data_arrays_0.data_arrays_0_ext.mem" "R270" 199880 100800

# re-cut the rows for where the macros actually sit, so the followpins stop at
# the macro edges instead of running through them
cut_rows

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

add_pdn_stripe -layer metal4 -width 0.48 -spacing 4.0 -pitch 49.0 -offset 2.5
add_pdn_stripe -layer metal7 -width 1.4 -pitch 40.0 -offset 2.5

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "sram_fe" \
  -instances "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem"
add_pdn_stripe -grid "sram_fe" -layer metal5 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5
add_pdn_stripe -grid "sram_fe" -layer metal6 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5

add_pdn_connect -grid "sram_fe" -layers {metal4 metal5}
add_pdn_connect -grid "sram_fe" -layers {metal5 metal6}
add_pdn_connect -grid "sram_fe" -layers {metal6 metal7}

define_pdn_grid -macro -name "sram_r270" \
  -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem"
add_pdn_stripe -grid "sram_r270" -layer metal5 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5
add_pdn_stripe -grid "sram_r270" -layer metal6 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5

add_pdn_connect -grid "sram_r270" -layers {metal4 metal5}
add_pdn_connect -grid "sram_r270" -layers {metal5 metal6}
add_pdn_connect -grid "sram_r270" -layers {metal6 metal7}

pdngen

set def_file [make_result_file macros_flipped_right_angle_180.def]
write_def $def_file
diff_files macros_flipped_right_angle_180.defok $def_file

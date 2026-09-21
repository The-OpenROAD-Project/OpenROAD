# test that a right-angle rotated instance's straps honor the flip, not the
# rotation
#
# A strap's direction comes from its layer, which is fixed in the die frame and
# cannot turn with the instance, so a rotated macro's grid has to be written in
# the rotated frame to begin with.  R90 is that frame, and the offsets are
# measured against it:
#   - R90 (W) is the reference, so nothing moves
#   - MXR90 (FW) is R90 with the placed x mirrored, so the vertical (metal6)
#     offsets are measured from the right edge instead
# The horizontal (metal5) offsets stay put for both, so each macro is the
# control for the other's unmirrored axis.  macros_flipped_right_angle_180
# covers the two orientations that are a half turn from these, and
# macros_flipped_right_angle_halo covers -halo and ring -core_offsets, which
# name edges rather than directions and so do turn with the instance.
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

place_rotated "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem" "R90" 49970 100800
place_rotated "dcache.data.data_arrays_0.data_arrays_0_ext.mem" "MXR90" 199880 100800

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

define_pdn_grid -macro -name "sram_r90" \
  -instances "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem"
add_pdn_stripe -grid "sram_r90" -layer metal5 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5
add_pdn_stripe -grid "sram_r90" -layer metal6 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5

add_pdn_connect -grid "sram_r90" -layers {metal4 metal5}
add_pdn_connect -grid "sram_r90" -layers {metal5 metal6}
add_pdn_connect -grid "sram_r90" -layers {metal6 metal7}

define_pdn_grid -macro -name "sram_fw" \
  -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem"
add_pdn_stripe -grid "sram_fw" -layer metal5 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5
add_pdn_stripe -grid "sram_fw" -layer metal6 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5

add_pdn_connect -grid "sram_fw" -layers {metal4 metal5}
add_pdn_connect -grid "sram_fw" -layers {metal5 metal6}
add_pdn_connect -grid "sram_fw" -layers {metal6 metal7}

pdngen

set def_file [make_result_file macros_flipped_right_angle.def]
write_def $def_file
diff_files macros_flipped_right_angle.defok $def_file

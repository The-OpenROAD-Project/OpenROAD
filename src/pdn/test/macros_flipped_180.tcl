# test that instance grids honor an R180 instance (mirrored on both axes)
#
# The first macro is rotated 180 degrees, which mirrors both x and y; the second
# keeps its as-drawn R0 orientation and uses the identical recipe, so it is the
# control.  Everything that is given per-edge or as an offset from the macro
# origin has to mirror for the R180 instance:
#   - the strap offsets on both metal5 (horizontal) and metal6 (vertical)
#   - the net order inside each strap group, on both axes
#   - -halo and -core_offsets {1 2 3 4} -> {3 4 1 2}
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

# rotate the macro in place; the footprint is unchanged, only the internal
# geometry mirrors.  FIXED instances have to be released to be re-oriented.
proc flip_inst { name orient } {
  set inst [[ord::get_db_block] findInst $name]
  set status [$inst getPlacementStatus]
  $inst setPlacementStatus PLACED
  $inst setLocationOrient $orient
  $inst setPlacementStatus $status
}

flip_inst "dcache.data.data_arrays_0.data_arrays_0_ext.mem" "R180"

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1
add_pdn_stripe -layer metal4 -width 0.48 -spacing 4.0 -pitch 49.0 -offset 2.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 40.0 -offset 2.0

add_pdn_connect -grid "Core" -layers {metal1 metal4}
add_pdn_connect -grid "Core" -layers {metal4 metal7}

define_pdn_grid -macro -name "sram_r180" \
  -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem" \
  -halo {1.0 2.0 3.0 4.0}
add_pdn_ring -grid "sram_r180" -layers {metal5 metal6} -widths 2.0 -spacings 2.0 \
  -core_offsets {1.0 2.0 3.0 4.0}
add_pdn_stripe -grid "sram_r180" -layer metal5 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5 \
  -extend_to_core_ring
add_pdn_stripe -grid "sram_r180" -layer metal6 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5 \
  -extend_to_core_ring

add_pdn_connect -grid "sram_r180" -layers {metal4 metal5}
add_pdn_connect -grid "sram_r180" -layers {metal5 metal6}
add_pdn_connect -grid "sram_r180" -layers {metal6 metal7}

define_pdn_grid -macro -name "sram_r0" \
  -instances "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem" \
  -halo {1.0 2.0 3.0 4.0}
add_pdn_ring -grid "sram_r0" -layers {metal5 metal6} -widths 2.0 -spacings 2.0 \
  -core_offsets {1.0 2.0 3.0 4.0}
add_pdn_stripe -grid "sram_r0" -layer metal5 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5 \
  -extend_to_core_ring
add_pdn_stripe -grid "sram_r0" -layer metal6 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5 \
  -extend_to_core_ring

add_pdn_connect -grid "sram_r0" -layers {metal4 metal5}
add_pdn_connect -grid "sram_r0" -layers {metal5 metal6}
add_pdn_connect -grid "sram_r0" -layers {metal6 metal7}

pdngen -report_only

pdngen

set def_file [make_result_file macros_flipped_180.def]
write_def $def_file
diff_files macros_flipped_180.defok $def_file

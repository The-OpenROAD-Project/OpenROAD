# test that -number_of_straps counts from the mirrored edge on a flipped instance
#
# -number_of_straps stops adding straps once the count is met, so on a flipped
# instance it has to count from the edge the offset is measured from:
#   - MY mirrors x, so the two metal6 groups belong at the right edge
#   - MX mirrors y, so the two metal5 groups belong at the top edge
# The layer that is not mirrored keeps counting from the low edge, which makes
# each macro the control for the other axis.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

# flip the macros in place; the footprint is unchanged, only the internal
# geometry mirrors.  FIXED instances have to be released to be re-oriented.
proc flip_inst { name orient } {
  set inst [[ord::get_db_block] findInst $name]
  set status [$inst getPlacementStatus]
  $inst setPlacementStatus PLACED
  $inst setLocationOrient $orient
  $inst setPlacementStatus $status
}

flip_inst "dcache.data.data_arrays_0.data_arrays_0_ext.mem" "MY"
flip_inst "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem" "MX"

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

define_pdn_grid -macro -name "sram_flip_x" \
  -instances "dcache.data.data_arrays_0.data_arrays_0_ext.mem"
add_pdn_stripe -grid "sram_flip_x" -layer metal5 -width 0.93 -spacing 1.5 -pitch 15.0 \
  -offset 2.5 -number_of_straps 2
add_pdn_stripe -grid "sram_flip_x" -layer metal6 -width 0.93 -spacing 1.5 -pitch 6.0 \
  -offset 1.5 -number_of_straps 2

add_pdn_connect -grid "sram_flip_x" -layers {metal4 metal5}
add_pdn_connect -grid "sram_flip_x" -layers {metal5 metal6}
add_pdn_connect -grid "sram_flip_x" -layers {metal6 metal7}

define_pdn_grid -macro -name "sram_flip_y" \
  -instances "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem"
add_pdn_stripe -grid "sram_flip_y" -layer metal5 -width 0.93 -spacing 1.5 -pitch 15.0 \
  -offset 2.5 -number_of_straps 2
add_pdn_stripe -grid "sram_flip_y" -layer metal6 -width 0.93 -spacing 1.5 -pitch 6.0 \
  -offset 1.5 -number_of_straps 2

add_pdn_connect -grid "sram_flip_y" -layers {metal4 metal5}
add_pdn_connect -grid "sram_flip_y" -layers {metal5 metal6}
add_pdn_connect -grid "sram_flip_y" -layers {metal6 metal7}

pdngen

set def_file [make_result_file macros_flipped_number_of_straps.def]
write_def $def_file
diff_files macros_flipped_number_of_straps.defok $def_file

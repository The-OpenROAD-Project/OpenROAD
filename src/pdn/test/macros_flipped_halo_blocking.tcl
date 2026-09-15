# test that the PDN-0008 halo suggestion is reported in the macro's own frame
#
# The macro is flipped on y, so the "top" halo the user asked for lands on the
# bottom of the placed instance.  The row clearance differs on the two sides, so
# the halo that gets clipped -- and the value PDN-0008 tells the user to type --
# has to be reported back in the as-drawn frame, not the placed one.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

# flip the macro in place; the footprint is unchanged, only the internal
# geometry mirrors.  FIXED instances have to be released to be re-oriented.
proc flip_inst { name orient } {
  set inst [[ord::get_db_block] findInst $name]
  set status [$inst getPlacementStatus]
  $inst setPlacementStatus PLACED
  $inst setLocationOrient $orient
  $inst setPlacementStatus $status
}

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

# the row clearance is 5.2 um below the macro and 6.0 um above it, so a top halo
# of 8 um is too big whichever way it is resolved, but by a different amount
define_pdn_grid -macro -name "sram_flip_y" \
  -instances "frontend.icache.data_arrays_0.data_arrays_0_0_ext.mem" \
  -halo {1.0 2.0 3.0 8.0}
add_pdn_stripe -grid "sram_flip_y" -layer metal5 -width 0.93 -spacing 1.5 -pitch 15.0 -offset 2.5
add_pdn_stripe -grid "sram_flip_y" -layer metal6 -width 0.93 -spacing 1.5 -pitch 6.0 -offset 1.5

add_pdn_connect -grid "sram_flip_y" -layers {metal4 metal5}
add_pdn_connect -grid "sram_flip_y" -layers {metal5 metal6}
add_pdn_connect -grid "sram_flip_y" -layers {metal6 metal7}

catch { pdngen } err
puts $err

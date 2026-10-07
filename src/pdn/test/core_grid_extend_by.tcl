# test add_pdn_stripe -extend_by
#
# Each layer exercises one combination:
#   metal1 followpins, -extend_by 2.0
#   metal4 straps, -extend_by 3.0
#   metal7 straps, -extend_to_core_ring -extend_by 1.0, so past the ring
#   metal8 straps, -extend_by 50.0, more than the space to the die
# The shapes are not trimmed, so each one keeps the length it was built with
# rather than being cut back to its last via.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_ring -layers {metal5 metal6} -widths 1.0 -spacings 1.0 -core_offsets 1.0

add_pdn_stripe -followpins -layer metal1 -extend_by 2.0
add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 -offset 5.0 -extend_by 3.0
add_pdn_stripe -layer metal7 -width 1.4 -pitch 20.0 -offset 5.0 \
  -extend_to_core_ring -extend_by 1.0
add_pdn_stripe -layer metal8 -width 1.4 -pitch 20.0 -offset 5.0 -extend_by 50.0

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}
add_pdn_connect -layers {metal7 metal8}

pdngen -skip_trim

set def_file [make_result_file core_grid_extend_by.def]
write_def $def_file
diff_files core_grid_extend_by.defok $def_file

# there are no pads here, so there is no pad ring to extend to
catch { add_pdn_stripe -layer metal9 -width 1.4 -pitch 20.0 -extend_to_pad_ring } err
puts $err

# the pad ring is one more place to extend to, so it excludes the others
catch { add_pdn_stripe -layer metal4 -width 0.48 -pitch 20.0 \
  -extend_to_boundary -extend_to_pad_ring } err
puts $err

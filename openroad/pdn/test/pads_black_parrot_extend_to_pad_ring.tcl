# test add_pdn_stripe -extend_to_pad_ring stops the straps at the inner edge of
# the pads, short of the die
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_bsg_black_parrot/dummy_pads.lef

read_def nangate_bsg_black_parrot/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"

add_pdn_stripe -followpins -layer metal1 -extend_to_pad_ring
add_pdn_stripe -layer metal8 -width 1.40 -pitch 200.0 -offset 2.70 -extend_to_pad_ring
add_pdn_stripe -layer metal9 -width 1.40 -pitch 200.0 -offset 2.70 -extend_to_pad_ring

add_pdn_connect -layers {metal8 metal9}

# The shapes are not trimmed, so each one keeps the length it was built with
# rather than being cut back to its last via.
pdngen -skip_trim

set def_file [make_result_file pads_black_parrot_extend_to_pad_ring.def]
write_def $def_file
diff_files pads_black_parrot_extend_to_pad_ring.defok $def_file

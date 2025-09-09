# test for connecting grid to existing routing (such as with the flipchip from PAD)
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_bsg_black_parrot/dummy_pads.lef

read_def nangate_bsg_black_parrot/floorplan_flipchip_nogrid.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -starts_with "POWER"
add_pdn_ring -grid "Core" -layers {metal8 metal9} -widths 5.0 \
  -spacings 2.0 -core_offsets 4.5 -connect_to_pads
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2.70 -extend_to_core_ring
add_pdn_stripe -layer metal8 -width 1.40 -pitch 40.0 -offset 2.70 -extend_to_core_ring
add_pdn_stripe -layer metal9 -width 1.40 -pitch 40.0 -offset 2.70 -extend_to_core_ring

add_pdn_connect -layers {metal7 metal8}
add_pdn_connect -layers {metal8 metal9}
add_pdn_connect -layers {metal9 metal10}

define_pdn_grid -name "Bumps" -macro -instances bump_*
add_pdn_ring -grid "Bumps" -layers {metal9 metal10} -widths 5.0 -spacings 2.0 \
  -core_offsets 5 -add_connect
add_pdn_stripe -grid "Bumps" -layer metal10 -width 5 -pitch 40.0 -offset 25 \
  -number_of_straps 1 -extend_to_core_ring
add_pdn_connect -grid "Bumps" -layers {metal8 metal9}

pdngen

set def_file [make_result_file pads_black_parrot_flipchip_connect_bumps.def]
write_def $def_file
diff_files pads_black_parrot_flipchip_connect_bumps.defok $def_file

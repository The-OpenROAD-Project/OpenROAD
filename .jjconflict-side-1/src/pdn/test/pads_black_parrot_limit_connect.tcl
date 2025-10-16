# test for add_pdn_ring -connect_to_pads -connect_to_pad_layers
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

add_pdn_ring -grid "Core" -layers {metal8 metal9} -widths 5.0 -spacings 2.0 \
  -pad_offsets 4.5 -connect_to_pads -connect_to_pad_layers metal7

add_pdn_connect -layers {metal7 metal8}
add_pdn_connect -layers {metal7 metal9}
add_pdn_connect -layers {metal8 metal9}

pdngen

set def_file [make_result_file pads_black_parrot_limit_connect.def]
write_def $def_file
diff_files pads_black_parrot_limit_connect.defok $def_file

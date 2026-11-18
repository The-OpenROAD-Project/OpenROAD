# Regression for edge connections cut off their pins by the obstructions of
# the padframe.
#
# nangate_bsg_black_parrot/dummy_pads_blanket_obs.lef draws the power pads and
# the fillers between them that way.  The outermost pins of a power pad are
# 1.335 um from its side, inside the 1.5 um spacing of the filler beside it.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_bsg_black_parrot/dummy_pads_blanket_obs.lef
read_lef nangate_bsg_black_parrot/dummy_pads.lef

read_def nangate_bsg_black_parrot/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -starts_with "POWER"
add_pdn_ring -grid "Core" -layers {metal8 metal9} -widths 5.0 \
  -spacings 2.0 -core_offsets 2 -connect_to_pads

add_pdn_connect -layers {metal8 metal9}

pdngen

set def_file [make_result_file pads_connect_blanket_obstruction.def]
write_def $def_file
diff_files pads_connect_blanket_obstruction.defok $def_file

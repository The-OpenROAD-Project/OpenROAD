# test add_pdn_stripe -offset_type, with each type on its own layer
#
# Every layer uses the same two-net recipe: 1.0 um straps 2.0 um apart, so the
# group is 4.0 um wide, offset 10.0 um from the core.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

add_pdn_stripe -layer metal4 -width 1.0 -spacing 2.0 -pitch 20.0 -offset 10.0 \
  -offset_type START
add_pdn_stripe -layer metal5 -width 1.0 -spacing 2.0 -pitch 20.0 -offset 10.0 \
  -offset_type FIRST
add_pdn_stripe -layer metal6 -width 1.0 -spacing 2.0 -pitch 20.0 -offset 10.0 \
  -offset_type CENTER
add_pdn_stripe -layer metal7 -width 1.0 -spacing 2.0 -pitch 20.0 -offset 10.0 \
  -offset_type LAST
add_pdn_stripe -layer metal8 -width 1.0 -spacing 2.0 -pitch 20.0 -offset 10.0 \
  -offset_type END

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal5}
add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal6 metal7}
add_pdn_connect -layers {metal7 metal8}

pdngen

set def_file [make_result_file core_grid_offset_type.def]
write_def $def_file
diff_files core_grid_offset_type.defok $def_file

# an unknown type is rejected
catch { add_pdn_stripe -layer metal4 -width 1.0 -pitch 40.0 -offset_type MIDDLE } err
puts $err

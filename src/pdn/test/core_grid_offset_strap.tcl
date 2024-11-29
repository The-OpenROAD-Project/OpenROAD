# test for vias on partially overlapping straps
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_def sky130_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VDD -pin_pattern VPWR
add_global_connection -net VSS -pin_pattern VSS -ground
add_global_connection -net VSS -pin_pattern VGND

set_voltage_domain -power VDD -ground VSS

define_pdn_grid \
  -name stdcell_grid \
  -starts_with POWER \
  -voltage_domain CORE \
  -pins "met4 met5"

add_pdn_stripe \
  -grid stdcell_grid \
  -layer met4 \
  -width 1.6 \
  -pitch 22.44 \
  -offset 11.44 \
  -starts_with POWER

add_pdn_stripe \
  -grid stdcell_grid \
  -layer met5 \
  -width 1.6 \
  -pitch 22.44 \
  -offset 11.22 \
  -starts_with POWER

add_pdn_connect \
  -grid stdcell_grid \
  -layers "met4 met5"

add_pdn_stripe \
  -grid stdcell_grid \
  -layer met1 \
  -width 0.48 \
  -followpins

add_pdn_connect \
  -grid stdcell_grid \
  -layers "met1 met4"

pdngen

set def_file [make_result_file core_grid_offset_strap.def]
write_def $def_file
diff_files core_grid_offset_strap.defok $def_file

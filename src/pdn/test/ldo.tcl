source "helpers.tcl"

read_lef ../../../test/sky130hvl/sky130_fd_sc_hvl.tlef 
read_lef ../../../test/sky130hvl/sky130_fd_sc_hvl_merged.lef 

read_lef sky130hvl/capacitor_test_nf.lef  
read_lef sky130hvl/LDO_COMPARATOR_LATCH.lef
read_lef sky130hvl/PMOS.lef
read_lef sky130hvl/PT_UNIT_CELL.lef
read_lef sky130hvl/vref_gen_nmos_with_trim.lef

read_def ldo/2_5_floorplan_tapcell.def

add_global_connection -net VREG -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VDD -inst_pattern {pt_array_unit.*} -pin_pattern VPB 
add_global_connection -net VDD -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VSS -inst_pattern {pt_array_unit.*} -pin_pattern VGND -ground
add_global_connection -net VSS -inst_pattern {pt_array_unit.*} -pin_pattern VNB

pdngen ldo/pdn.cfg -verbose

set def_file results/ldo.def
write_def $def_file 

diff_files $def_file ldo.defok

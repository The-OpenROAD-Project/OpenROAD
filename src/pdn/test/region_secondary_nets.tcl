# test for set_voltage_domain -secondary_power
source "helpers.tcl"

read_lef sky130hvl/sky130_fd_sc_hvl.tlef
read_lef sky130hvl/sky130_fd_sc_hvl_merged.lef
read_lef sky130_secondary_nets/capacitor_test_nf.lef
read_lef sky130_secondary_nets/LDO_COMPARATOR_LATCH.lef
read_lef sky130_secondary_nets/PMOS.lef
read_lef sky130_secondary_nets/PT_UNIT_CELL.lef
read_lef sky130_secondary_nets/vref_gen_nmos_with_trim.lef

read_def sky130_secondary_nets/floorplan.def

add_global_connection -net VDD -inst_pattern {.*} -pin_pattern VPB -power
add_global_connection -net VDD -inst_pattern {.*} -pin_pattern VPWR
add_global_connection -net VDD -inst_pattern {.*} -pin_pattern vpwr
add_global_connection -net VDD -inst_pattern {.*} -pin_pattern vnb
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern VGND -ground
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern VNB
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern vgnd
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern vpb
add_global_connection -net VREG1 -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VREG1 -inst_pattern {.*} -pin_pattern VREG
add_global_connection -net VREG2 -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power

set_voltage_domain -power VDD -ground VSS
set_voltage_domain -power VDD -ground VSS -region "test_domain" -secondary_power "VREG1 VREG2"

define_pdn_grid -name "Core" -voltage_domains "Core"
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_ring -grid "Core" -layers {met4 met5} -widths 5.0 -spacings 2.0 -core_offsets 4.5
add_pdn_stripe -layer met4 -width 1.6 -pitch 126.0 -offset 2.0 -extend_to_core_ring

add_pdn_connect -grid "Core" -layers {met1 met4}
add_pdn_connect -grid "Core" -layers {met4 met5}

define_pdn_grid -name "Region" -voltage_domains "test_domain"
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_ring -grid "Region" -layers {met4 met5} -widths 5.0 -spacings 2.0 -core_offsets 4.5
add_pdn_stripe -layer met4 -width 1.6 -pitch 10.0 -spacing 0.5 -offset 2.0 -extend_to_core_ring

add_pdn_connect -grid "Region" -layers {met1 met4}
add_pdn_connect -grid "Region" -layers {met4 met5}

pdngen

set def_file [make_result_file region_secondary_nets.def]
write_def $def_file
diff_files region_secondary_nets.defok $def_file

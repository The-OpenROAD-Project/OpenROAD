# test for errors when domain and grid names are used more than once
source "helpers.tcl"

read_lef sky130hvl/sky130_fd_sc_hvl.tlef
read_lef sky130hvl/sky130_fd_sc_hvl_merged.lef
read_lef sky130_secondary_nets/capacitor_test_nf.lef
read_lef sky130_secondary_nets/LDO_COMPARATOR_LATCH.lef
read_lef sky130_secondary_nets/PMOS.lef
read_lef sky130_secondary_nets/PT_UNIT_CELL.lef
read_lef sky130_secondary_nets/vref_gen_nmos_with_trim.lef

read_def sky130_secondary_nets/floorplan.def

add_global_connection -net VREG1 -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VREG2 -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VDD -inst_pattern {pt_array_unit.*} -pin_pattern VPB -power
add_global_connection -net VDD -inst_pattern {pt_array_unit.*} -pin_pattern VPWR
add_global_connection -net VSS -inst_pattern {pt_array_unit.*} -pin_pattern VGND -ground
add_global_connection -net VSS -inst_pattern {pt_array_unit.*} -pin_pattern VNB

set_voltage_domain -power VDD -ground VSS
# Warning for overwriting core domain
set_voltage_domain -power VDD -ground VSS

set_voltage_domain -power VDD -ground VSS -region "test_domain" -secondary_power "VREG1 VREG2"
# tclint-disable-next-line line-length
catch {set_voltage_domain -power VDD -ground VSS -region "test_domain" -secondary_power "VREG1 VREG2"} err
puts $err

define_pdn_grid -name "Core" -voltage_domains "Core"
catch {define_pdn_grid -name "Core" -voltage_domains "Core"} err
puts $err

define_pdn_grid -macro -name "Inst" -voltage_domains "Core" -instances {cmp1}
catch {define_pdn_grid -macro -name "Inst" -voltage_domains "Core" -instances {cmp1}}
puts $err

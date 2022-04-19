# test for power switch, which is not implemented
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef 
read_lef sky130hd/sky130_fd_sc_hd_merged.lef 
read_lef sky130_power_switch/power_switch.lef 

read_def sky130_power_switch/floorplan.def

add_global_connection -net VDD -pin_pattern VDDG -power
add_global_connection -net VDD_SW -pin_pattern VPWR -power
add_global_connection -net VDD_SW -pin_pattern VPB
add_global_connection -net VSS -pin_pattern VGND -ground
add_global_connection -net VSS -pin_pattern VNB

set_voltage_domain -name CORE -power VDD -ground VSS -switched_power VDD_SW

define_pdn_grid -name grid -switch_cell POWER_SWITCH
add_pdn_stripe -followpins -layer met1 -width 0.48
add_pdn_stripe -layer met4 -width 1.600 -pitch 27.140 -offset 13.570
add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600
add_pdn_connect -layers {met1 met4} 
add_pdn_connect -layers {met4 met5}

pdngen

set def_file [make_result_file power_switch.def]
write_def $def_file
diff_files power_switch.defok $def_file

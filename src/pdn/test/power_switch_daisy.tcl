# test the insertion of power switches into a design.
# The power switch control is connected in a DAISY configuration with avoidance of tapcells
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_power_switch/power_switch.lef

read_def sky130_power_switch/floorplan.def

add_global_connection -net VDD -power -pin_pattern "^VDDG$"
add_global_connection -net VDD_SW -power -pin_pattern "^VPB$"
add_global_connection -net VDD_SW -pin_pattern "^VPWR$"
add_global_connection -net VSS -power -pin_pattern "^VGND$"
add_global_connection -net VSS -power -pin_pattern "^VNB$"

set_voltage_domain -power VDD -ground VSS -switched_power VDD_SW
define_power_switch_cell -name POWER_SWITCH -control SLEEP -acknowledge SLEEP_OUT \
  -power_switchable VPWR -power VDDG -ground VGND
define_pdn_grid -name "Core" -power_switch_cell POWER_SWITCH -power_control nPWRUP \
  -power_control_network DAISY

add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 20.000 -offset 13.000
add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600
add_pdn_connect -layers {met1 met4}
add_pdn_connect -layers {met2 met4}
add_pdn_connect -layers {met4 met5}

pdngen

set def_file [make_result_file power_switch_daisy.def]
write_def $def_file
diff_files power_switch_daisy.defok $def_file

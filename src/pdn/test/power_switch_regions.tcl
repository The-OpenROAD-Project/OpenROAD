# test the insertion of power switches into a design with regions
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_temp_sensor/HEADER.lef
read_lef sky130_temp_sensor/SLC.lef
read_lef sky130_power_switch/power_switch.lef

read_def sky130_power_switch/floorplan_regions.def

add_global_connection -net VDD -pin_pattern "^VDDG$" -power
add_global_connection -net VDD -pin_pattern VPWR
add_global_connection -net VDD -pin_pattern VPB
add_global_connection -net VSS -pin_pattern VGND -ground
add_global_connection -net VSS -pin_pattern VNB

add_global_connection -net VDD -inst_pattern {temp_analog_1.*} -pin_pattern VPWR \
  -region TEMP_ANALOG
add_global_connection -net VDD -inst_pattern {temp_analog_1.*} -pin_pattern VPB \
  -region TEMP_ANALOG
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPWR -power \
  -region TEMP_ANALOG
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPB \
  -region TEMP_ANALOG

define_power_switch_cell -name POWER_SWITCH -control SLEEP -acknowledge SLEEP_OUT \
  -power_switchable VPWR -power VDDG -ground VGND

set_voltage_domain -power VDD -ground VSS -switched_power VDD_SW

define_pdn_grid -name "Core" -power_switch_cell POWER_SWITCH -power_control nPWRUP0 \
  -power_control_network STAR
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_stripe -layer met4 -width 1.6 -pitch 50.0 -offset 13.570 -extend_to_core_ring
add_pdn_stripe -layer met5 -width 1.6 -pitch 27.2 -offset 13.600 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {met4 met5} -widths 2.0 -spacings 2.0 -core_offsets 2

add_pdn_connect -grid "Core" -layers {met1 met4}
add_pdn_connect -grid "Core" -layers {met4 met5}

set_voltage_domain -power VIN -ground VSS -region "TEMP_ANALOG" -switched_power VIN_SW
define_pdn_grid -name "TempSensor" -voltage_domains "TEMP_ANALOG" \
  -power_switch_cell POWER_SWITCH -power_control nPWRUP1 -power_control_network DAISY
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_stripe -layer met4 -width 1.6 -spacing 1.6 -pitch 50.0 -offset 13.570 -extend_to_core_ring
add_pdn_stripe -layer met5 -width 1.6 -spacing 1.6 -pitch 27.2 -offset 13.600 -extend_to_core_ring

add_pdn_ring -grid "TempSensor" -layers {met4 met5} -widths 2.0 -spacings 2.0 -core_offsets 2

add_pdn_connect -grid "TempSensor" -layers {met1 met4}
add_pdn_connect -grid "TempSensor" -layers {met4 met5}

pdngen

set def_file [make_result_file power_switch_regions.def]
write_def $def_file
diff_files power_switch_regions.defok $def_file

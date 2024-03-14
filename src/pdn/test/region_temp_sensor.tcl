# test for set_voltage_domain -region
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_temp_sensor/HEADER.lef
read_lef sky130_temp_sensor/SLC.lef

read_def sky130_temp_sensor/floorplan.def

add_global_connection -net VDD -inst_pattern {.*} -pin_pattern VPWR -power
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern VGND -ground
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern VNB

add_global_connection -net VDD -inst_pattern {temp_analog_1.*} -pin_pattern VPWR
add_global_connection -net VDD -inst_pattern {temp_analog_1.*} -pin_pattern VPB
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPWR -power
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPB

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_stripe -layer met4 -width 1.6 -pitch 50.0 -offset 13.570 -extend_to_core_ring
add_pdn_stripe -layer met5 -width 1.6 -pitch 27.2 -offset 13.600 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {met4 met5} -widths 5.0 -spacings 2.0 -core_offsets 4.5

add_pdn_connect -grid "Core" -layers {met1 met4}
add_pdn_connect -grid "Core" -layers {met4 met5}

set_voltage_domain -power VIN -ground VSS -region "TEMP_ANALOG"
define_pdn_grid -name "TempSensor" -voltage_domains "TEMP_ANALOG"
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_stripe -layer met4 -width 1.6 -spacing 1.6 -pitch 50.0 -offset 13.570 -extend_to_core_ring
add_pdn_stripe -layer met5 -width 1.6 -spacing 1.6 -pitch 27.2 -offset 13.600 -extend_to_core_ring

add_pdn_ring -grid "TempSensor" -layers {met4 met5} -widths 5.0 -spacings 2.0 -core_offsets 3.5

add_pdn_connect -grid "TempSensor" -layers {met1 met4}
add_pdn_connect -grid "TempSensor" -layers {met4 met5}

pdngen

set def_file [make_result_file region_temp_sensor.def]
write_def $def_file
diff_files region_temp_sensor.defok $def_file

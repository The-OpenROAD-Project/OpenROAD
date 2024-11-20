# test the insertion of power switches into a design with regions
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_temp_sensor/HEADER.lef
read_lef sky130_temp_sensor/SLC.lef
read_lef sky130_power_switch/power_switch.lef

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog sky130_power_switch/netlist_regions.v
link_design tempsenseInst

create_power_domain PD_TOP \
  -elements {.}

create_logic_port nPWRUP0
create_logic_port nPWRUP1

create_power_domain TEMP_ANALOG

set_domain_area TEMP_ANALOG -area {15 15 50 50}

create_power_switch PS_0 \
  -domain PD_TOP \
  -output_supply_port {vout VDD_SW} \
  -input_supply_port {vin VDD} \
  -control_port {sleep nPWRUP0} \
  -ack_port acknowledge

map_power_switch PS_0 \
  -lib_cells POWER_SWITCH \
  -port_map {{vout VPWR} {vin VDDG} {sleep SLEEP} {acknowledge SLEEP_OUT} {ground VGND}}

create_power_switch PS_1 \
  -domain TEMP_ANALOG \
  -output_supply_port {vout VIN_SW} \
  -input_supply_port {vin VIN} \
  -control_port {sleep nPWRUP1} \
  -ack_port acknowledge

map_power_switch PS_1 \
  -lib_cells POWER_SWITCH \
  -port_map {{vout VPWR} {vin VDDG} {sleep SLEEP} {acknowledge SLEEP_OUT} {ground VGND}}

initialize_floorplan -utilization 10 -core_space 0.0 -site unithd -additional_site unithddbl
tapcell \
  -distance 14 \
  -tapcell_master "sky130_fd_sc_hd__tapvpwrvgnd_1"

add_global_connection -net VDD -power -pin_pattern "^VDDG$"
add_global_connection -net VDD_SW -power -pin_pattern "^VPB$"
add_global_connection -net VDD_SW -pin_pattern "^VPWR$"
add_global_connection -net VSS -power -pin_pattern "^VGND$"
add_global_connection -net VSS -power -pin_pattern "^VNB$"

add_global_connection -net VIN_SW -inst_pattern {temp_analog_1.*} -pin_pattern VPWR \
  -region TEMP_ANALOG
add_global_connection -net VIN_SW -inst_pattern {temp_analog_1.*} -pin_pattern VPB \
  -region TEMP_ANALOG
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPWR -power \
  -region TEMP_ANALOG
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPB \
  -region TEMP_ANALOG

define_power_switch_cell -name POWER_SWITCH -control SLEEP -acknowledge SLEEP_OUT \
  -power_switchable VPWR -power VDDG -ground VGND

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -power_control_network STAR
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_stripe -layer met4 -width 1.6 -pitch 50.0 -offset 13.570 -extend_to_core_ring
add_pdn_stripe -layer met5 -width 1.6 -pitch 27.2 -offset 13.600 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {met4 met5} -widths 2.0 -spacings 2.0 -core_offsets 2

add_pdn_connect -grid "Core" -layers {met1 met4}
add_pdn_connect -grid "Core" -layers {met4 met5}

set_voltage_domain -power VIN -ground VSS -region "TEMP_ANALOG"
define_pdn_grid -name "TempSensor" -voltage_domains "TEMP_ANALOG" \
  -power_control_network DAISY
add_pdn_stripe -followpins -layer met1 -width 0.48 -extend_to_core_ring
add_pdn_stripe -layer met4 -width 1.6 -spacing 1.6 -pitch 50.0 -offset 13.570 -extend_to_core_ring
add_pdn_stripe -layer met5 -width 1.6 -spacing 1.6 -pitch 27.2 -offset 13.600 -extend_to_core_ring

add_pdn_ring -grid "TempSensor" -layers {met4 met5} -widths 2.0 -spacings 2.0 -core_offsets 2

add_pdn_connect -grid "TempSensor" -layers {met1 met4}
add_pdn_connect -grid "TempSensor" -layers {met4 met5}

pdngen

set def_file [make_result_file power_switch_upf_regions.def]
write_def $def_file
diff_files power_switch_upf_regions.defok $def_file

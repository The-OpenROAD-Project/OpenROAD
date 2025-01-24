# test the insertion of power switches into a design.
# The power switch control is connected in a STAR configuration
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_power_switch/power_switch.lef

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog sky130_power_switch/netlist.v
link_design gcd

create_power_domain PD_TOP \
  -elements {.}

create_logic_port nPWRUP

create_power_switch PS_1 \
  -domain PD_TOP \
  -output_supply_port {vout VDD_SW} \
  -input_supply_port {vin VDD} \
  -control_port {sleep nPWRUP} \
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

set_voltage_domain -power VDD -ground VSS
define_pdn_grid -name "Core" -power_control_network DAISY

add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 27.140 -offset 13.570
add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600
add_pdn_connect -layers {met1 met4}
add_pdn_connect -layers {met2 met4}
add_pdn_connect -layers {met4 met5}

pdngen

set def_file [make_result_file power_switch_upf_daisy.def]
write_def $def_file
diff_files power_switch_upf_daisy.defok $def_file

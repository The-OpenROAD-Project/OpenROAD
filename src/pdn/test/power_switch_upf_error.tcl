# test if UPF is loaded, information must come from UPF
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_power_switch/power_switch.lef

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib

read_verilog sky130_power_switch/netlist.v
link_design gcd

create_power_domain PD_TOP \
  -elements {.}

initialize_floorplan -utilization 10 -core_space 0.0 -site unithd -additional_site unithddbl
tapcell \
  -distance 14 \
  -tapcell_master "sky130_fd_sc_hd__tapvpwrvgnd_1"

add_global_connection -net VDD -power -pin_pattern "^VDDG$"
add_global_connection -net VDD_SW -power -pin_pattern "^VPB$"
add_global_connection -net VDD_SW -pin_pattern "^VPWR$"
add_global_connection -net VSS -power -pin_pattern "^VGND$"
add_global_connection -net VSS -power -pin_pattern "^VNB$"

catch {set_voltage_domain -power VDD -ground VSS -switched_power VDD_SW} err
puts $err

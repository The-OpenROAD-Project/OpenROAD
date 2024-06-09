# test for error at non rectangular region
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef sky130_non_rect_region/HEADER.lef
read_lef sky130_non_rect_region/SLC.lef

read_def sky130_non_rect_region/floorplan.def

add_global_connection -net VDD -inst_pattern {temp_analog_1.*} -pin_pattern VPWR -power
add_global_connection -net VDD -inst_pattern {temp_analog_1.*} -pin_pattern VPB
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPWR -power
add_global_connection -net VIN -inst_pattern {temp_analog_0.*} -pin_pattern VPB
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern VGND -ground
add_global_connection -net VSS -inst_pattern {.*} -pin_pattern VNB

set_voltage_domain -power VDD -ground VSS
catch {set_voltage_domain -power VIN -ground VSS -region "TEMP_ANALOG"} err
puts $err

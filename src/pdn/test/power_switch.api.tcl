source "helpers.tcl"

read_lef ../../../test/sky130hd/sky130hd.tlef 
read_lef ../../../test/sky130hd/sky130_fd_sc_hd_merged.lef 

read_lef sky130hd/power_switch.lef 

read_def power_switch/2_5_floorplan_tapcell.def

set ::halo 4

# POWER or GROUND #Upper metal stripes starting with power or ground rails at the left/bottom of the core area
set ::stripes_start_with "POWER" ;

set ::rails_start_with "POWER" ;

set ::power_nets "VDD VDD_SW"
set ::ground_nets "VSS"

set ::core_domain "CORE"
# Voltage domain
set_voltage_domain -name CORE -power VDD -ground VSS -switched_power VDD_SW

add_global_connection -net VDD -pin_pattern VDDG -power
add_global_connection -net VDD_SW -pin_pattern VPB
add_global_connection -net VDD_SW -pin_pattern VPWR -power
add_global_connection -net VSS -pin_pattern VGND -ground
add_global_connection -net VSS -pin_pattern VNB

define_pdn_grid -name grid -switch_cell POWER_SWITCH
add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 27.140 -offset 13.570
add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600
add_pdn_connect -layers {met1 met4} 
add_pdn_connect -layers {met4 met5}

define_pdn_grid -macro -orient {R0 R180 MX MY} -pin_direction vertical
add_pdn_connect -layers {met4 met5}

# Need a different strategy for rotated rams to connect to rotated pins
# No clear way to do this for a 5 metal layer process
define_pdn_grid -macro -orient {R90 R270 MXR90 MYR90} -pin_direction horizontal
add_pdn_connect -layers {met4 met5}

pdngen -verbose

set def_file results/power_switch.api.def
write_def $def_file 

diff_files $def_file power_switch.defok
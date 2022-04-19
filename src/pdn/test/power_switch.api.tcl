# test the insertion of power switches into a design. The power switch control is connected in a STAR configuration
source "helpers.tcl"

read_lef sky130hd/sky130hd.tlef 
read_lef sky130hd/sky130_fd_sc_hd_merged.lef 

read_lef sky130_power_switch/power_switch.lef 

read_def sky130_power_switch/floorplan.def

set ::halo 4

# POWER or GROUND #Upper metal stripes starting with power or ground rails at the left/bottom of the core area
set ::stripes_start_with "POWER" ;

set ::rails_start_with "POWER" ;

set ::power_nets "VDD VDD_SW"
set ::ground_nets "VSS"

set ::core_domain "CORE"
# Voltage domain
pdngen::set_voltage_domain -name CORE -power VDD -ground VSS -switched_power VDD_SW

set pdngen::global_connections {
  VDD {
    {inst_name ".*" pin_name "^VDDG$"}
  }
  VDD_SW {
    {inst_name ".*" pin_name "^VPB$"}
    {inst_name ".*" pin_name "^VPWR$"}
  }
  VSS {
    {inst_name ".*" pin_name "^VGND$"}
    {inst_name ".*" pin_name "^VNB$"}
  }
}

pdngen::define_power_switch_cell -name POWER_SWITCH -control SLEEP -acknowledge SLEEP_OUT -power_switchable VDD -power VDDG -ground VGND

pdngen::define_pdn_grid -name grid -switch_cell POWER_SWITCH -power_control nPWRUP -power_control_network STAR
pdngen::add_pdn_stripe -layer met1 -width 0.48 -offset 0 -followpins
pdngen::add_pdn_stripe -layer met4 -width 1.600 -pitch 27.140 -offset 13.570
pdngen::add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600
pdngen::add_pdn_connect -layers {met1 met4} 
pdngen::add_pdn_connect -layers {met4 met5}

pdngen::apply  

set def_file results/power_switch.api.def
write_def $def_file 

diff_files $def_file power_switch.api.defok

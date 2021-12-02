source "helpers.tcl"

read_lef ../../../test/sky130hvl/sky130_fd_sc_hvl.tlef 
read_lef ../../../test/sky130hvl/sky130_fd_sc_hvl_merged.lef 
read_lef sky130hvl/capacitor_test_nf.lef
read_lef sky130hvl/LDO_COMPARATOR_LATCH.lef
read_lef sky130hvl/PMOS.lef
read_lef sky130hvl/PT_UNIT_CELL.lef
read_lef sky130hvl/vref_gen_nmos_with_trim.lef

read_def ldo/2_5_floorplan_tapcell.def

set ::halo 4

# POWER or GROUND #Upper metal stripes starting with power or ground rails at the left/bottom of the core area
set ::stripes_start_with "POWER" ;

set ::rails_start_with "POWER" ;

set ::power_nets "VDD VREG"
set ::ground_nets "VSS"

set ::core_domain "CORE"
# Voltage domain
set_voltage_domain -name CORE -power VDD -ground VSS
set_voltage_domain -name test_domain -power VDD -ground VSS -secondary_power VREG

add_global_connection -net VREG -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VDD -inst_pattern {pt_array_unit.*} -pin_pattern VPB
add_global_connection -net VDD -inst_pattern {pt_array_unit.*} -pin_pattern VPWR -power
add_global_connection -net VSS -inst_pattern {pt_array_unit.*} -pin_pattern VGND -ground
add_global_connection -net VSS -inst_pattern {pt_array_unit.*} -pin_pattern VNB

define_pdn_grid -name grid
add_pdn_stripe -layer met1 -width 0.48 -offset 0 -extend_to_core_ring -followpins
add_pdn_stripe -layer met4 -width 1.600 -pitch 50.000 -offset 13.570
#add_pdn_stripe -layer met5 -width 1.600 -pitch 27.200 -offset 13.600

add_pdn_ring -layers {met4 met5} -widths 5.0 -spacings 2.0 -core_offsets 4.5
add_pdn_connect -layers {met1 met4} 
add_pdn_connect -layers {met4 met5}

define_pdn_grid -macro -orient {R0 R180 MX MY} -pin_direction vertical
add_pdn_connect -layers {met4 met5}

# Need a different strategy for rotated rams to connect to rotated pins
# No clear way to do this for a 5 metal layer process
define_pdn_grid -macro -orient {R90 R270 MXR90 MYR90} -pin_direction horizontal
add_pdn_connect -layers {met4 met5}

pdngen -verbose

set def_file results/ldo.api.def
write_def $def_file 

diff_files $def_file ldo.api.defok

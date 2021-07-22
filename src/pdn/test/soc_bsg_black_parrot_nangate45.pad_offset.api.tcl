source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_lef dummy_pads.lef

read_def soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.init.def


# Floorplan information - core boundary coordinates, std. cell row height,
# minimum track pitch as defined in LEF

set ::halo 4

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

define_pdn_grid -name grid

add_pdn_ring -layers {metal8 metal9}  -widths 5.0 -spacings 2.0 -pad_offsets 4.5 -power_pads {PADCELL_VDD_V PADCELL_VDD_H} -ground_pads {PADCELL_VSS_V PADCELL_VSS_H}

add_pdn_stripe -layer metal1 -width 0.17 -pitch  2.4 -offset 0 -followpins
add_pdn_stripe -layer metal4 -width 0.48 -pitch 56.0 -offset 2
add_pdn_stripe -layer metal7 -width 1.40 -pitch 40.0 -offset 2
add_pdn_stripe -layer metal8 -width 1.40 -pitch 40.0 -offset 2
add_pdn_stripe -layer metal9 -width 1.40 -pitch 40.0 -offset 2

add_pdn_connect -layers {metal1 metal4} 
add_pdn_connect -layers {metal4 metal7} 
add_pdn_connect -layers {metal7 metal8} 
add_pdn_connect -layers {metal8 metal9}

define_pdn_grid -macro -orient {R0 R180 MX MY} -pin_direction vertical

add_pdn_stripe -layer metal5 -width 0.93 -pitch 40.0 -offset 2
add_pdn_stripe -layer metal6 -width 0.93 -pitch 40.0 -offset 2

add_pdn_connect -layers {metal4 metal5} 
add_pdn_connect -layers {metal5 metal6} 
add_pdn_connect -layers {metal6 metal7}

define_pdn_grid -macro -orient {R90 R270 MXR90 MYR90} -pin_direction horizontal
add_pdn_stripe -layer metal6 -width 0.93 -pitch 40.0 -offset 2
add_pdn_connect -layers {metal4 metal6} 
add_pdn_connect -layers {metal6 metal7}

define_pdn_grid -macro -name bumps -cells "PAD"
define_pdn_grid -macro -name cdmm -cells "MARKER"

pdngen -verbose

set def_file results/soc_bsg_black_parrot_nangate45_pdn.pad_offset.def
write_def $def_file

diff_files $def_file soc_bsg_black_parrot_nangate45.pad_offset.defok

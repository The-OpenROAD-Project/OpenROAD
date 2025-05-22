# test for repair channel a single channel repair creating a new channel which is fixable
source "helpers.tcl"

read_lef ihp_ethmac/tech.lef
read_lef ihp_ethmac/sg13g2.lef
read_lef ihp_ethmac/RM_IHPSG13_1P_256x48_c2_bm_bist.lef

read_def ihp_ethmac/floorplan.def
####################################
# voltage domains
####################################
set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}
#####################################
# standard cell grid
####################################
define_pdn_grid -name {grid} -voltage_domains {CORE} -pins {TopMetal2}
add_pdn_stripe -grid {grid} -layer {Metal1} -width {0.44} -followpins
set met5_pitch [expr {([lindex [ord::get_core_area] 3] - [lindex [ord::get_core_area] 1]) / 2}]
if {$met5_pitch > 75.6} {
    set met5_pitch 75.6
}
set top1_pitch [expr {([lindex [ord::get_core_area] 2] - [lindex [ord::get_core_area] 0]) / 2}]
if {$top1_pitch > 75.6} {
    set top1_pitch 75.6
}
set top2_pitch [expr {([lindex [ord::get_core_area] 3] - [lindex [ord::get_core_area] 1]) / 2}]
if {$top2_pitch > 75.6} {
    set top2_pitch 75.6
}

proc snap_grid {value} {
    set grid [[ord::get_db_tech] getManufacturingGrid]
    set dbus [[ord::get_db_tech] getDbUnitsPerMicron]

    set val_dbus [ord::microns_to_dbu $value]
    set val_snapped [expr {$grid * round($val_dbus / $grid)}]

    return [ord::dbu_to_microns $val_snapped]
}

add_pdn_stripe -grid {grid} -layer {Metal5} -width {2.200} -pitch [snap_grid $met5_pitch] \
    -offset [snap_grid [expr {$met5_pitch / 2}]]
add_pdn_stripe -grid {grid} -layer {TopMetal1} -width {2.000} -pitch [snap_grid $top1_pitch] \
    -offset [snap_grid [expr {$top1_pitch / 2}]]
add_pdn_stripe -grid {grid} -layer {TopMetal2} -width {2.000} -pitch [snap_grid $top2_pitch] \
    -offset [snap_grid [expr {$top2_pitch / 2}]]
add_pdn_connect -grid {grid} -layers {Metal1 Metal5}
add_pdn_connect -grid {grid} -layers {Metal5 TopMetal1}
add_pdn_connect -grid {grid} -layers {TopMetal1 TopMetal2}

define_pdn_grid -name {sg13g2_sram_R0} -voltage_domains {CORE} -macro \
    -orient {R0 R180 MX MY} \
    -halo {1.0 1.0 1.0 1.0} \
    -cells {RM_IHPSG13_1P_.*}
add_pdn_stripe -grid {sg13g2_sram_R0} -layer {TopMetal1} -width {2.000} -pitch {20.0} -offset {1.0}
add_pdn_connect -grid {sg13g2_sram_R0} -layers {Metal4 TopMetal1}
add_pdn_connect -grid {sg13g2_sram_R0} -layers {TopMetal1 TopMetal2}

define_pdn_grid -name {sg13g2_sram_R90} -voltage_domains {CORE} -macro \
    -orient {R90 R270 MXR90 MYR90} \
    -halo {1.0 1.0 1.0 1.0} \
    -cells {RM_IHPSG13_1P_.*}
add_pdn_stripe -grid {sg13g2_sram_R90} -layer {Metal5} -width {2.200} -pitch {22.0} -offset {11.0}
add_pdn_connect -grid {sg13g2_sram_R90} -layers {Metal4 Metal5}
add_pdn_connect -grid {sg13g2_sram_R90} -layers {Metal5 TopMetal1}
add_pdn_connect -grid {sg13g2_sram_R90} -layers {TopMetal1 TopMetal2}

pdngen

set def_file [make_result_file macros_channel_recursive_repair.def]
write_def $def_file
diff_files macros_channel_recursive_repair.defok $def_file

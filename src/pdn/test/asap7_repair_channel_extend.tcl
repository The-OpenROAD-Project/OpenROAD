# Regression for ORFS #4539 (asap7 cva6): an island of rows reachable only
# through a repair channel was left with no power and no diagnostic.
#
# The island (x 3.24-4.32, y 14.58-16.47) falls between the regular M5 straps
# (x 1.38, 6.78, ...) and between the M6 straps (y 12.633, 18.033, ...), so M2
# has no M5 strap to reach and a repair channel is needed on M5. That repair
# strap has to be grown until it reaches the M6 straps of its own net, on both
# sides, or it connects the island to itself and to nothing else. Reaching the
# nearest shape of any net is not enough: the VDD and VSS M6 straps are offset
# from each other, so a strap that stops at the neighbouring net's edge is
# still floating.
#
# M6 is declared as the pin layer, as asap7 does. The guard that keeps repair
# straps from becoming block pins is exercised by macros_channel_recursive_repair,
# where a repair does land on the pin layer; here the M5 repair reaches the grid
# so no repair strap is created on M6 at all.
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_repair_channel_pin_layer.def

add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
global_connect

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid -name {top} -voltage_domains {CORE} -pins {M6}
add_pdn_stripe -grid {top} -layer {M1} -width {0.018} -pitch {0.54} -offset {0} -followpins
add_pdn_stripe -grid {top} -layer {M2} -width {0.018} -pitch {0.54} -offset {0} -followpins
add_pdn_stripe -grid {top} -layer {M5} -width {0.12} -spacing {0.072} -pitch {5.4} -offset {0.300}
add_pdn_stripe -grid {top} -layer {M6} -width {0.288} -spacing {0.096} -pitch {5.4} -offset {0.513}
add_pdn_connect -grid {top} -layers {M1 M2}
add_pdn_connect -grid {top} -layers {M2 M5}
add_pdn_connect -grid {top} -layers {M5 M6}

pdngen

set def_file [make_result_file asap7_repair_channel_extend.def]
write_def $def_file
diff_files asap7_repair_channel_extend.defok $def_file

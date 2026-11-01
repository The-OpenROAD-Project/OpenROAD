# Followpin rails over the row pattern of a hybrid site, with the parent rows
# removed.
#
# Companion to gf180_hybrid_followpins, which reads the floorplan exactly as
# initialize_floorplan writes it: the 8 rows of the sc9sc7 row pattern plus 4
# rows of the parent sc9sc7 site itself, overlapping them.  Those parent rows
# are 8.96um tall and would contribute rails of their own, so the two things
# that have to be handled -- rows of heights that share no pitch, and rows
# that stand in for other rows -- land on top of each other.
#
# gf180_hybrid/floorplan_rowpattern.def is the same floorplan with the 4 parent
# rows deleted, leaving only the alternating 9-track (5.04um) and 7-track
# (3.92um) rows.  It covers the pitch on its own:
#
#   row                        y range        rails it owns
#   GF018hv5v_green_sc9   8.96 - 14.00   VSS at  8.96, VDD at 14.00
#   GF018hv5v_mcu_sc7    14.00 - 17.92   VDD at 14.00, VSS at 17.92
#   GF018hv5v_green_sc9  17.92 - 22.96   VSS at 17.92, VDD at 22.96
#   ...
#
# Each row must contribute rails at its own two edges and nowhere else, so the
# rails are VSS at 8.96, VDD at 14.00, VSS at 17.92, VDD at 22.96, VSS at
# 26.88, VDD at 31.92, VSS at 35.84, VDD at 40.88 and VSS at 44.80um.  A
# 5.04um row stepped by the 3.92um standard cell row would instead put a rail
# at 12.88um, inside itself and over the cells.
source "helpers.tcl"

read_lef gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef
read_lef gf180/gf180mcu_fd_sc_mcu7t5v0.lef
read_lef gf180_hybrid/hybrid_sites.lef
read_lef gf180_hybrid/adjusted_sc9.lef
read_def gf180_hybrid/floorplan_rowpattern.def

add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
global_connect

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid -name {block} -voltage_domains {CORE}
add_pdn_stripe -grid {block} -layer {Metal1} -width {0.600} -offset {0} -followpins
add_pdn_stripe -grid {block} -layer {Metal4} -width {1.600} -pitch {20.000} -offset {10.000}
add_pdn_connect -grid {block} -layers {Metal1 Metal4}

pdngen

set def_file [make_result_file gf180_hybrid_followpins_rowpattern.def]
write_def $def_file
diff_files gf180_hybrid_followpins_rowpattern.defok $def_file

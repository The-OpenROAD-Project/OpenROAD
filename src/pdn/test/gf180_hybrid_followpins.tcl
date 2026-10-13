# Followpin rails over hybrid rows, modelled on the OpenROAD-flow-scripts
# design gf180/aes-hybrid.
#
# gf180 has no hybrid rows of its own; ORFS uses it as a public proxy for an
# advanced process by defining a hybrid site
#
#   SITE sc9sc7 SIZE 0.56 BY 8.96 ROWPATTERN GF018hv5v_green_sc9 N
#                                            GF018hv5v_mcu_sc7   FS ;
#
# and placing with PLACE_SITE = sc9sc7, so a 9-track (5.04um) row and a
# 7-track (3.92um) row alternate all the way up the core.
#
# gf180_hybrid/floorplan.def was produced by
#   initialize_floorplan -die_area {0 0 44.8 53.76} \
#                        -core_area {5.6 8.96 39.2 44.8} -site sc9sc7
# which emits the 8 rows of the row pattern *and* 4 rows of the parent sc9sc7
# site that overlap them -- so the database holds rows of three different
# heights (3.92, 5.04 and 8.96um) covering the same area.
#
# No single pitch describes that floorplan, so each row has to contribute the
# rails at its own two edges:
#
#   * The rows of the pattern give VSS at 8.96, VDD at 14.00, VSS at 17.92,
#     VDD at 22.96, VSS at 26.88, VDD at 31.92, VSS at 35.84, VDD at 40.88 and
#     VSS at 44.80um -- spacing alternating 5.04 / 3.92um.  A rail placed a
#     fixed stride from a row's lower edge would land inside the 5.04um rows
#     instead, over the cells.
#   * The parent sc9sc7 rows carry no rails of their own.  They stand in for
#     the pattern that has already been expanded beneath them, and being
#     8.96um tall they cannot describe the boundary in their middle.
#
# The Metal1 followpin width is the ORFS one (pdn_grid_strategy_7t_6M.cfg also
# passes -pitch 3.92, which followpins ignore in favour of the row height);
# the Metal4 straps are scaled down from the ORFS pitch to fit the test core.
#
# The rails must be exactly the nine listed above and nothing else: no rail
# may fall inside a row, and no boundary may be left bare.
source "helpers.tcl"

read_lef gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef
read_lef gf180/gf180mcu_fd_sc_mcu7t5v0.lef
read_lef gf180_hybrid/hybrid_sites.lef
read_lef gf180_hybrid/adjusted_sc9.lef
read_def gf180_hybrid/floorplan.def

add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
global_connect

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid -name {block} -voltage_domains {CORE}
add_pdn_stripe -grid {block} -layer {Metal1} -width {0.600} -offset {0} -followpins
add_pdn_stripe -grid {block} -layer {Metal4} -width {1.600} -pitch {20.000} -offset {10.000}
add_pdn_connect -grid {block} -layers {Metal1 Metal4}

pdngen

set def_file [make_result_file gf180_hybrid_followpins.def]
write_def $def_file
diff_files gf180_hybrid_followpins.defok $def_file

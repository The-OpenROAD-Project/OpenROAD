# The followpin pitch reported for a hybrid floorplan.
#
# gf180_hybrid/floorplan.def comes from the OpenROAD-flow-scripts design
# gf180/aes-hybrid, whose hybrid site sc9sc7 stacks a 9-track (5.04um) row
# under a 7-track (3.92um) row.  initialize_floorplan writes the two rows of
# the pattern plus the 8.96um parent row that spans them, so the design has
# rows of three heights over the same area and no single value describes the
# rail-to-rail spacing: it alternates 5.04um and 3.92um.
#
# The reported pitch is therefore only nominal -- 2x the shortest row, 7.84um.
# It is what channel repair bloats by, not what places the rails; those come
# from the rows themselves, see gf180_hybrid_followpins.
source "helpers.tcl"

read_lef gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef
read_lef gf180/gf180mcu_fd_sc_mcu7t5v0.lef
read_lef gf180_hybrid/hybrid_sites.lef
read_lef gf180_hybrid/adjusted_sc9.lef
read_def gf180_hybrid/floorplan.def

add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid -name {block} -voltage_domains {CORE}
add_pdn_stripe -grid {block} -layer {Metal1} -width {0.600} -offset {0} -followpins
add_pdn_stripe -grid {block} -layer {Metal4} -width {1.600} -pitch {20.000} -offset {10.000}
add_pdn_connect -grid {block} -layers {Metal1 Metal4}

pdngen -report_only

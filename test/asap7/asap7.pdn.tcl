# macro level counterpart to BLOCKS_grid_strategy.tcl

####################################
# global connections
####################################
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground

####################################
# voltage domains
####################################
set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

####################################
# standard cell grid
# M1 M2 are for follow pin, width derived from PG rail in standard cell
# M5 stripe width rerived from one of width allowed in LEF, offset and pitch
#   put stripe on M5 track
# M4 M5 ring follow stripe width
####################################
define_pdn_grid -name {top} -voltage_domains {CORE} -pins {M5}

add_pdn_ring -grid {top} -layers {M5 M4} -widths {0.12 0.12} -spacings {0.072} -core_offset {0.084}

add_pdn_stripe -grid {top} -layer {M1} -width {0.018} -pitch {0.54} -offset {0} -followpins
add_pdn_stripe -grid {top} -layer {M2} -width {0.018} -pitch {0.54} -offset {0} -followpins

add_pdn_stripe -grid {top} -layer {M5} -width {0.12} -spacing {0.072} -pitch {2.976} -offset {1.5} \
  -extend_to_core_ring

add_pdn_connect -grid {top} -layers {M1 M2}
add_pdn_connect -grid {top} -layers {M2 M5}
add_pdn_connect -grid {top} -layers {M4 M5}

####################################
# global connections
####################################
add_global_connection -defer_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -defer_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDDPE$}
add_global_connection -defer_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDDCE$}
add_global_connection -defer_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
add_global_connection -defer_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSSE$}
global_connect
####################################
# voltage domains
####################################
set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}
####################################
# standard cell grid
####################################
define_pdn_grid -name {grid} -voltage_domains {CORE}
add_pdn_stripe -grid {grid} -layer {metal1} -width {0.17} -pitch {2.4} -offset {0} -followpins
add_pdn_stripe -grid {grid} -layer {metal4} -width {0.48} -pitch {56.0} -offset {2}
add_pdn_stripe -grid {grid} -layer {metal7} -width {1.40} -pitch {40.0} -offset {2}
add_pdn_connect -grid {grid} -layers {metal1 metal4}
add_pdn_connect -grid {grid} -layers {metal4 metal7}
####################################
# macro grids
####################################
####################################
# grid for: CORE_macro_grid_1
####################################
define_pdn_grid -name {CORE_macro_grid_1} -voltage_domains {CORE} -macro -orient {R0 R180 MX MY} -halo {4.0 4.0 4.0 4.0} -default -grid_over_boundary
add_pdn_stripe -grid {CORE_macro_grid_1} -layer {metal5} -width {0.93} -pitch {10.0} -offset {2}
add_pdn_stripe -grid {CORE_macro_grid_1} -layer {metal6} -width {0.93} -pitch {10.0} -offset {2}
add_pdn_connect -grid {CORE_macro_grid_1} -layers {metal4 metal5}
add_pdn_connect -grid {CORE_macro_grid_1} -layers {metal5 metal6}
add_pdn_connect -grid {CORE_macro_grid_1} -layers {metal6 metal7}
####################################
# grid for: CORE_macro_grid_2
####################################
define_pdn_grid -name {CORE_macro_grid_2} -voltage_domains {CORE} -macro -orient {R90 R270 MXR90 MYR90} -halo {4.0 4.0 4.0 4.0} -default -grid_over_boundary
add_pdn_stripe -grid {CORE_macro_grid_2} -layer {metal6} -width {0.93} -pitch {40.0} -offset {2}
add_pdn_connect -grid {CORE_macro_grid_2} -layers {metal4 metal6}
add_pdn_connect -grid {CORE_macro_grid_2} -layers {metal6 metal7}

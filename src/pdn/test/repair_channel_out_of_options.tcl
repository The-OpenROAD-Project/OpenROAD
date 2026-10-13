# Channel repair must stop once a repair strap has run out of widths and
# spacings to try. A strap already at the layer minimum width whose spacing is
# still above the layer minimum used to be "continued" forever: the recomputed
# parameters were unchanged, the rebuild succeeded, and a successful rebuild
# counted as a repair, so repairGridChannels recursed until the stack ran out.
#
# Whittled from ORFS's nangate45 bp_quad floorplan, which never finished
# pdngen: standard cells dropped (followpins come from rows, not instances),
# everything outside the two failing channels removed, then translated to the
# origin. The translation moved rows, instances and tracks together, so the
# grid lands exactly where it did on the 3.6 mm die, 3451.42 um and 3437.115 um
# further out. The two metal1 channels below cannot be closed by any metal4
# strap, so the expected answer is PDN-0179.

read_lef Nangate45/Nangate45_tech.lef
read_lef repair_channel_out_of_options/fakeram45_32x32.lef
read_lef repair_channel_out_of_options/fakeram45_64x62.lef

read_def repair_channel_out_of_options/floorplan.def

add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDDPE$}
add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDDCE$}
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSSE$}
global_connect

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid -name {grid} -voltage_domains {CORE} -pins {metal7}
add_pdn_stripe -grid {grid} -layer {metal1} -width {0.17} -pitch {2.4} -offset {0} -followpins
add_pdn_stripe -grid {grid} -layer {metal4} -width {0.48} -pitch {56.0} -offset {2}
add_pdn_stripe -grid {grid} -layer {metal7} -width {1.40} -pitch {30.0} -offset {2}
add_pdn_connect -grid {grid} -layers {metal1 metal4}
add_pdn_connect -grid {grid} -layers {metal4 metal7}

define_pdn_grid -name {CORE_macro_grid_1} -voltage_domains {CORE} -macro \
  -orient {R0 R180 MX MY} -halo {2.0 2.0 2.0 2.0} -default
add_pdn_stripe -grid {CORE_macro_grid_1} -layer {metal5} -width {0.93} -pitch {10.0} -offset {2}
add_pdn_stripe -grid {CORE_macro_grid_1} -layer {metal6} -width {0.93} -pitch {10.0} -offset {2}
add_pdn_connect -grid {CORE_macro_grid_1} -layers {metal4 metal5}
add_pdn_connect -grid {CORE_macro_grid_1} -layers {metal5 metal6}
add_pdn_connect -grid {CORE_macro_grid_1} -layers {metal6 metal7}

catch { pdngen } err
puts $err

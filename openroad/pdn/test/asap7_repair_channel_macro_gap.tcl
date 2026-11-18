# Repair channels in the narrow gaps between rows of abutting macros.
#
# The macros block M5 across their whole footprint, so the only M5 left in a
# gap between two macro rows is the short piece spanning that gap. M6 has a
# 4.32 pitch while two of the gaps here are 2.16, so those gaps catch no M6
# stripe and their M5 pieces are left with nothing above them. Repairing them
# targets M6, which is the top of the stack.
#
# Nothing on M5 beneath such a repair is connected upward yet - providing that
# connection is the point of it - and the M5 pieces draw their power from the
# followpins below instead. A repair at the top of the stack must therefore not
# be made conditional on finding an already-connected strap beneath it, or
# these channels are refused and pdngen fails with PDN-0179 on a grid that is
# in fact repairable.
#
# The gaps are only found as channels after earlier repairs give their M5 a via
# down to the followpins, so the design needs enough macro rows for that
# cascade to play out; a couple of rows on their own repair in a single pass.
#
# The DEF golden pins the repair straps themselves: a run of unconnected straps
# is repaired with one strap across the whole run, not with a strap per piece
# of it, which used to leave holes in the M6 repairs.
source "helpers.tcl"

read_lef asap7_data/asap7_tech_1x_201209.lef
read_lef asap7_data/repair_channel_macro_gap_cells.lef
read_lef asap7_data/element.lef
read_def asap7_repair_channel_macro_gap.def

add_global_connection -net {VDD} -inst_pattern {.*} -pin_pattern {^VDD$} -power
add_global_connection -net {VSS} -inst_pattern {.*} -pin_pattern {^VSS$} -ground
global_connect

set_voltage_domain -name {CORE} -power {VDD} -ground {VSS}

define_pdn_grid -name {top} -voltage_domains {CORE} -pins {M6}
add_pdn_stripe -grid {top} -layer {M1} -width {0.018} -pitch {0.54} -offset {0} -followpins
add_pdn_stripe -grid {top} -layer {M2} -width {0.018} -pitch {0.54} -offset {0} -followpins
add_pdn_ring -grid {top} -layers {M5 M6} -widths {0.504 0.544} -spacings {0.096} \
  -core_offset {0.504}
add_pdn_stripe -grid {top} -layer {M5} -width {0.12} -spacing {0.072} -pitch {2.16} \
  -offset {1.50} -extend_to_core_ring
add_pdn_stripe -grid {top} -layer {M6} -width {0.288} -spacing {0.096} -pitch {4.32} \
  -offset {1.504} -extend_to_core_ring
add_pdn_connect -grid {top} -layers {M1 M2}
add_pdn_connect -grid {top} -layers {M2 M5}
add_pdn_connect -grid {top} -layers {M5 M6}

define_pdn_grid -macro -cells {Element} -halo {0.5 0.5 0.5 0.5} \
  -voltage_domains {CORE} -name {ElementGrid}
add_pdn_connect -grid {ElementGrid} -layers {M5 M6}

pdngen

set def_file [make_result_file asap7_repair_channel_macro_gap.def]
write_def $def_file
diff_files asap7_repair_channel_macro_gap.defok $def_file

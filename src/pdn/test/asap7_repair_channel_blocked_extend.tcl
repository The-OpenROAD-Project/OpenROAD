# Companion to asap7_repair_channel_extend: the same island, but with the M5
# path out of it obstructed in both directions, so the repair cannot escape on
# the layer it was built on.
#
# The two obstructions cover the M6 straps at y 12.633 and 18.033, which is
# where asap7_repair_channel_extend grows the M5 repair strap to. Growing M5 is
# therefore useless here: cutShapes trims it back to the island and it feeds
# nothing.
#
# The repair has to carry on up instead. M5 is not the top of the stack, so the
# short M5 strap is kept and findRepairChannels() picks it up on the next pass
# as a channel of its own, targeting M6. M6 *is* the top, so it cannot be fed
# from above and two parallel M6 straps never touch; it must instead reach a
# connected M5 strap crossing it, which it does by growing sideways to the
# straps at x 1.38 and 6.78. If it could not reach one, the repair would be
# rejected and the channel reported rather than left floating.
#
# M6 is the pin layer here, so this also covers the guard that keeps the two
# M6 repair straps from being written out as block pins.
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_repair_channel_pin_layer.def

create_obstruction -layer M5 -region {2.5 17 6 19.2}
create_obstruction -layer M5 -region {2.5 11.5 6 13.2}

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

set def_file [make_result_file asap7_repair_channel_blocked_extend.def]
write_def $def_file
diff_files asap7_repair_channel_blocked_extend.defok $def_file

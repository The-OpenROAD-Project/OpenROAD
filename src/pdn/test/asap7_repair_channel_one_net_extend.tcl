# Companion to asap7_repair_channel_blocked_extend, where only *one* net is
# walled in. The obstruction starts at x 3.85, past the VDD repair strap at
# x 3.6-3.72 but across the VSS one at x 3.792-3.912, so VDD grows out to its
# M6 straps normally and only VSS is left short.
#
# That leaves a repair channel holding a single strap. The rule that lets a
# lone strap be skipped must not apply to it: an ordinary strap on its own is
# usually about to be dropped anyway, but a repair strap exists because
# something needed feeding, so everything behind it floats if it is skipped.
# Without that distinction VSS is silently left unconnected here while VDD is
# fine, which is the asymmetry this test pins down.
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_repair_channel_pin_layer.def

create_obstruction -layer M5 -region {3.85 17 6 19.2}
create_obstruction -layer M5 -region {3.85 11.5 6 13.2}

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

set def_file [make_result_file asap7_repair_channel_one_net_extend.def]
write_def $def_file
diff_files asap7_repair_channel_one_net_extend.defok $def_file

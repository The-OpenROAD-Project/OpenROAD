# test that a layer occupied only by a ring is a valid -pins layer, and that the ring's own
# segments become block pins there.
#
# Grid::checkSetup() validates a -pins layer against a set built from the grid's rings AND its
# straps:
#
#     for (const auto& ring : rings_) { for (auto* layer : ring->getLayers()) ... }
#     for (const auto& strap : straps_) { check_layers.insert(strap->getLayer()); }
#
# Every other case in this directory that uses -pins names a STRAP layer, so the straps_ loop
# alone satisfies the check and the rings_ loop is uncovered: removing it leaves the suite green.
#
# This is core_grid_with_rings_with_straps with one option added -- -pins metal6, the upper layer
# of its ring -- so that case is the control, and the two goldens differ only where the option
# acts: two block terminals, two ports each, and unchanged SPECIALNETS geometry.
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_def nangate_gcd/floorplan.def

add_global_connection -net VDD -pin_pattern VDD -power
add_global_connection -net VSS -pin_pattern VSS -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core" -pins metal6
add_pdn_stripe -followpins -layer metal1 -extend_to_core_ring

add_pdn_stripe -layer metal4 -width 1.0 -pitch 5.0 -offset 2.5 -extend_to_core_ring

add_pdn_ring -grid "Core" -layers {metal5 metal6} -widths 2.0 -spacings 2.0 -core_offsets 2.0

add_pdn_connect -layers {metal5 metal6}
add_pdn_connect -layers {metal1 metal6}
add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal5}

pdngen

set def_file [make_result_file core_grid_ring_on_pin_layer.def]
write_def $def_file
diff_files core_grid_ring_on_pin_layer.defok $def_file

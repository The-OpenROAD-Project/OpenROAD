source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_aes.v
link_design mpd_top
read_upf -file upf/mpd_aes.upf
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

set_domain_area PD_AES_1 -area {30   30 650 490}
set_domain_area PD_AES_2 -area {30 510 650 970}


initialize_floorplan \
  -die_area {0 0 1000 1000} \
  -core_area {30 30 970 970} \
  -site unithd \
  -additional_site unithddbl

make_tracks

set_routing_layers -signal li1-met5

place_pins \
  -hor_layers met3 \
  -ver_layers met2
global_placement -skip_initial_place -density uniform -routability_driven -timing_driven

detailed_placement -max_displacement 650
improve_placement
check_placement

# Region semantics -- fence type, group membership, domain hierarchy -- are
# checked directly and far more thoroughly in
# src/upf/test/cpp/TestPowerDomainRegions.cc. What is left here is the part
# only a real design can show: that set_domain_area's micron arguments reach
# the database as the right DBU rectangle, and that placement honours the
# resulting fences, which check_placement above asserts (it errors on
# region_placement violations).
#
# Deliberately no .ok/.defok diff: those goldens recorded placer output rather
# than test intent and were a steady source of merge conflicts. They are still
# in the tree, unread by anything, only so that this test's conversion does not
# conflict with the in-flight PRs that regenerate them; deleting them is a
# follow-up (#11281).
set block [ord::get_db_block]
foreach { domain llx lly urx ury } {
  PD_AES_1 30 30 650 490
  PD_AES_2 30 510 650 970
} {
  set region [$block findRegion $domain]
  if { $region == "NULL" } {
    error "no region for power domain $domain"
  }
  set boundaries [$region getBoundaries]
  if { [llength $boundaries] != 1 } {
    error "$domain has [llength $boundaries] boundaries, expected 1"
  }
  set box [lindex $boundaries 0]
  set expected [list [ord::microns_to_dbu $llx] [ord::microns_to_dbu $lly] \
    [ord::microns_to_dbu $urx] [ord::microns_to_dbu $ury]]
  set actual [list [$box xMin] [$box yMin] [$box xMax] [$box yMax]]
  if { $actual != $expected } {
    error "$domain boundary is $actual, expected $expected"
  }
}

puts "pass"

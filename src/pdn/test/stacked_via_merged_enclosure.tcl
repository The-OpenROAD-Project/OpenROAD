# Cut enclosures on the intermediate layers of a via stack must satisfy the
# width-conditioned LEF58_ENCLOSURE tier selected by the metal that is actually
# drawn there -- the union of the pads landing on it from the cut layer below
# and the cut layer above -- not by the stripe intersection or by either pad on
# its own.  See The-OpenROAD-Project/OpenROAD#11129.
#
# MET1 (0.2 wide) crossing MET3 (1.0 wide) gives a 1.0 x 0.2 intersection, so
# V12 would pick its unconditioned ABOVE tier of 0.000 x 0.055.  V23 draws a
# 0.82 x 0.32 pad on MET2 though, and that merged MET2 shape is 0.320 wide --
# past the 0.300 threshold that makes V12 require 0.050 x 0.050.  via1_2 must
# come out with the 0.050 x 0.050 enclosure, and with its cut count intact.
source "helpers.tcl"

read_lef via_stack_enclosure/tech.lef
read_def via_stack_enclosure/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -layer MET1 -width 0.2 -pitch 2.0 -offset 1.0 -extend_to_boundary
add_pdn_stripe -layer MET3 -width 1.0 -pitch 10 -offset 5 -extend_to_boundary

add_pdn_connect -layers {MET1 MET3}

pdngen

set def_file [make_result_file stacked_via_merged_enclosure.def]
write_def $def_file
diff_files stacked_via_merged_enclosure.defok $def_file

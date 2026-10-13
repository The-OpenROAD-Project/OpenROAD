# Via generation between an M1 followpin and a narrower M2 followpin.
#
# V1 carries "ENCLOSURE CUTCLASS V1 BELOW 0.009 0.0", and the M1 strap is
# 0.054 wide, so M1 has 0.018 of room on each side of the 0.018 cut -- twice
# what the rule asks for.  That room lies outside the M1/M2 intersection
# though, because M2 is only as wide as the cut.  Measuring the enclosure from
# the intersection alone reports 0.0 for M1 and drops every via on the grid.
source "helpers.tcl"

read_lef asap7_vias/asap7_tech_1x_followpin_enc.lef
read_lef asap7_vias/asap7sc7p5t_27_R_1x.lef
read_def asap7_vias/floorplan.def

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VSS -pin_pattern {^VSS$} -ground

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer M1 -width 0.054
add_pdn_stripe -followpins -layer M2 -width 0.018

add_pdn_connect -layers {M1 M2}

pdngen

set def_file [make_result_file asap7_M1_M2_followpin_enclosure.def]
write_def $def_file
diff_files asap7_M1_M2_followpin_enclosure.defok $def_file

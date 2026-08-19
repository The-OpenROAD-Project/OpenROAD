# Regression test for https://github.com/The-OpenROAD-Project/OpenROAD/issues/10995
#
# A standard cell with exactly one non-power pin (here a tie-high cell whose
# only signal pin is a small multi-shape Metal1/Metal2/Via1 pin) could fail
# pin access with DRT-0087/DRT-0085. FlexPA::getEdgeCost's viol_access_points
# check was placed after the isSource()/isSink() early return, but those are
# the only kind of edge that exist in the DP graph when an instance has just
# one real pin, so a rejected access point was never actually avoided on
# retry.
#
# The single instance and its placement below (including the absolute
# coordinates, which affect access-point/track grid alignment) are taken
# directly from the design that hit this bug, whittled down to the minimum
# needed to reproduce it.
source "helpers.tcl"
read_lef "gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef"
read_lef "pa_single_pin.lef"
read_def "pa_single_pin.def"
pin_access -verbose 0

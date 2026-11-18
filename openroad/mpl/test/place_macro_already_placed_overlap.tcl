# Test if running place_macro on an already placed macro does not consider
# that same macro during overlap check.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/macro_only.lef"

read_def "./testcases/place_macro_already_placed_overlap.def"

place_macro -macro_name macro -location {100 100}

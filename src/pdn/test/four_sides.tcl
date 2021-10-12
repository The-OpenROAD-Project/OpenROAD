source "helpers.tcl"

set test_name four_sides
source four_sides/pdn.tcl
diff_files four_sides/pdn.def four_sides/pdn.defok

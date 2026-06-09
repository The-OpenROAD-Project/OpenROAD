# gcd global placement legalized with the negotiation legalizer.
# Overlapping (global-placed) cells exercise the full negotiation loop:
# overflow counting, history-cost update, negotiation-order sort and the
# rip-up/replace sweep over multiple iterations.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd_replace.def
detailed_placement -use_negotiation
check_placement

set def_file [make_result_file negotiation01.def]
write_def $def_file
diff_file negotiation01.defok $def_file

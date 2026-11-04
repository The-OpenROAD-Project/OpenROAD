# Test that the detour metric's initial Steiner wirelength is refreshed only
# for the nets touched by incremental global routing (CUGR).
# Moving _599_ changes the pins of _285_ and _286_; other nets keep their
# values from the full route.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

global_route -use_cugr

report_net_detours -top_n 3
report_net_detours -net {_285_ _286_ _284_}

global_route -start_incremental -use_cugr
set inst [[ord::get_db_block] findInst _599_]
lassign [$inst getLocation] x y
$inst setLocation [expr { $x + 20000 }] $y
global_route -end_incremental -use_cugr

report_net_detours -net {_285_ _286_ _284_}
report_net_detours -top_n 3

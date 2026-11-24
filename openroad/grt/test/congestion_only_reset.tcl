# Verify that -congestion_only applies to that global_route only: a later
# global route that does not ask for it, here the raw grt::global_route a C++
# caller such as global placement uses, routes in full and writes guides.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

global_route -congestion_only -allow_congestion

grt::global_route

set guides 0
foreach net [[ord::get_db_block] getNets] {
  incr guides [llength [$net getGuides]]
}
check "A global route after -congestion_only writes guides" {
  expr { $guides > 0 }
} 1

exit_summary

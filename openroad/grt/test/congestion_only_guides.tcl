# Verify that global_route -congestion_only leaves the router able to load
# the block's existing route guides afterwards: grt::have_routes and an
# incremental global route read them back as after read_db.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

global_route -allow_congestion

proc guide_count { } {
  set count 0
  foreach net [[ord::get_db_block] getNets] {
    incr count [llength [$net getGuides]]
  }
  return $count
}
set guides_before [guide_count]

global_route -congestion_only -allow_congestion

check "-congestion_only keeps the block's route guides" {
  expr { [guide_count] == $guides_before }
} 1

check "Route guides load after -congestion_only" {
  grt::have_routes
} 1

global_route -start_incremental
global_route -end_incremental

check "An incremental route after -congestion_only keeps the guides" {
  expr { [guide_count] == $guides_before }
} 1

exit_summary

source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_thread_count 16

if { [grt::get_snapshot_batched_width] != 0 } {
  utl::error GRT 707 \
    "FastRoute should start with snapshot_batched_width set to 0."
}

global_route -verbose -snapshot_batched_width 16

if { [grt::get_snapshot_batched_width] != 16 } {
  utl::error GRT 708 \
    "Default global_route should leave snapshot_batched_width at 16."
}

global_route -start_incremental
global_route -end_incremental -snapshot_batched_width 16

if { [grt::get_snapshot_batched_width] != 16 } {
  utl::error GRT 709 \
    "Incremental global_route -snapshot_batched_width 16 did not reach FastRoute."
}

global_route -start_incremental
global_route -end_incremental -snapshot_batched_width 0

if { [grt::get_snapshot_batched_width] != 0 } {
  utl::error GRT 710 \
    "Incremental global_route -snapshot_batched_width 0 did not disable snapshot-batched routing."
}

puts "pass"
exit 0

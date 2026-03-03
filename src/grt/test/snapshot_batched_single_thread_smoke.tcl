source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set_thread_count 1

set output ""
tee -variable output -quiet {
  global_route \
    -snapshot_batched_width 16 \
    -verbose
}

if {[grt::get_snapshot_batched_width] != 16} {
  utl::error GRT 711 \
    "global_route -snapshot_batched_width 16 should keep snapshot-batched routing enabled even with one requested thread."
}

set wirelength -1
set vias -1

foreach line [split $output "\n"] {
  if { [regexp {Total wirelength:\s+([0-9]+)\s+um} $line -> value] } {
    set wirelength $value
  } elseif { [regexp {Final number of vias:\s+([0-9]+)} $line -> value] } {
    set vias $value
  }
}

if { $wirelength <= 0 || $vias < 0 } {
  utl::error GRT 712 \
    "Failed to capture single-thread snapshot-batched global-route summary output."
}

puts "pass"
exit 0

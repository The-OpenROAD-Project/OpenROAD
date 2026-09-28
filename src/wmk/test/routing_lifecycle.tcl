# Tag changes after pin_access must reach the router's cached nets.
source "helpers.tcl"

proc route_case { case } {
  # Each case starts with a fresh router and the same unrouted design.
  set script [make_result_file routing_lifecycle/$case.tcl]
  set log [make_result_file routing_lifecycle/$case.log]
  set stream [open $script w]
  puts $stream [list set case $case]
  puts $stream [list source [file normalize routing_lifecycle_flow.tcl]]
  close $stream
  exec [info nameofexecutable] -no_splash -no_init -exit $script > $log 2>@1
  set stream [open $log r]
  set output [read $stream]
  close $stream
  if { ![regexp -line {^ROUTING_RESULT (.+)$} $output unused result] } {
    error "Missing routing result for $case; see $log"
  }
  return [lindex $result 0]
}

set results [dict create]
foreach case { unmarked early late cleared false neutral key_a key_b rekey_ab rekey_ba } {
  dict set results $case [route_case $case]
}

set ordinary [dict get $results unmarked]
set marked [dict get $results early]
check "the unmarked control has no tags" { dict get $ordinary nets } {}
check "the positive control tags both signal nets" { llength [dict get $marked nets] } 2
check "the controls route nonempty signal wires" {
  expr { [dict get $ordinary total] > 0 && [dict get $marked total] > 0 }
} 1
check "watermark bias reduces wrong-way routing" {
  expr { [dict get $marked wrong_way] < [dict get $ordinary wrong_way] }
} 1
check "tagging after pin access matches tagging before it" {
  dict get $results late
} $marked
foreach case { cleared false } {
  check "$case tags restore ordinary routing" { dict get $results $case } $ordinary
}
check "neutral strength retains the tags" {
  dict get $results neutral nets
} [dict get $marked nets]
foreach metric { total wrong_way } {
  check "neutral strength preserves ordinary $metric" {
    dict get $results neutral $metric
  } [dict get $ordinary $metric]
}

# These keys select opposite single-net sets. Check that the fixture still
# distinguishes their routing before using them as rekeying controls.
set key_a [dict get $results key_a]
set key_b [dict get $results key_b]
check "each key selects one signal net" {
  expr { [llength [dict get $key_a nets]] == 1 && [llength [dict get $key_b nets]] == 1 }
} 1
check "the keys select different signal nets" {
  expr { [dict get $key_a nets] ne [dict get $key_b nets] }
} 1
check "the key controls produce different wrong-way routing" {
  expr { [dict get $key_a wrong_way] != [dict get $key_b wrong_way] }
} 1
check "rekeying from A to B refreshes both cached flags" {
  dict get $results rekey_ab
} $key_b
check "rekeying from B to A refreshes both cached flags" {
  dict get $results rekey_ba
} $key_a
exit_summary

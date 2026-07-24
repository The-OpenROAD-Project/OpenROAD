# Shared helpers for the report_incremental_sta correctness-oracle tests.

# Load machine-readable incremental_sta_report_lines into an array
# (endpoint -> slack), skipping the SUMMARY line.
proc load_inc_slacks { lines arr_name } {
  upvar 1 $arr_name arr
  foreach line [lrange $lines 1 end] {
    set ep  [lindex $line 0]
    set slk [lindex $line 1]
    set arr($ep) $slk
  }
}

# Run the incremental-vs-full correctness oracle for the currently
# loaded+edited design.
#   seeds : list of edited instance paths.
# Returns "wns_ok tns_ok ep_ok affected_endpoints total_endpoints".
#
# Step 1 reads slacks INCREMENTALLY (only the invalidated cone is recomputed
# by OpenSTA's lazy query). Step 2 wipes ALL cached timing with delays_invalid
# and recomputes FULL from scratch on the same edited netlist. The two must be
# bit-for-bit identical on every reported value.
proc inc_vs_full_oracle { seeds } {
  set inc_lines [sta::incremental_sta_report_lines $seeds "max"]
  set inc_sum   [lindex $inc_lines 0]
  set inc_wns   [lindex $inc_sum 4]
  set inc_tns   [lindex $inc_sum 5]
  set aff       [lindex $inc_sum 2]
  set tot       [lindex $inc_sum 3]
  array set inc_slack {}
  load_inc_slacks $inc_lines inc_slack

  sta::delays_invalid
  set full_lines [sta::incremental_sta_report_lines $seeds "max"]
  set full_sum   [lindex $full_lines 0]
  set full_wns   [lindex $full_sum 4]
  set full_tns   [lindex $full_sum 5]
  array set full_slack {}
  load_inc_slacks $full_lines full_slack

  set wns_ok [expr { abs($inc_wns - $full_wns) < 1e-15 }]
  set tns_ok [expr { abs($inc_tns - $full_tns) < 1e-15 }]
  set ep_ok 1
  foreach ep [array names full_slack] {
    if { ![info exists inc_slack($ep)] } { set ep_ok 0; continue }
    if { abs($inc_slack($ep) - $full_slack($ep)) > 1e-15 } { set ep_ok 0 }
  }
  if { [array size inc_slack] != [array size full_slack] } { set ep_ok 0 }
  return [list $wns_ok $tns_ok $ep_ok $aff $tot]
}

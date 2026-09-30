# A claim file proves nothing to anyone who does not hold the key it was
# written with.
#
# Verification derives every target from the stage key.  With any other key
# the file's records disagree with what the key derives, and the file is
# refused rather than scored -- and a file written to match the design by
# someone without the key is refused the same way, because its bits are the
# design's and not the key's.  This is the property the whole scheme rests on:
# the marks are unforgeable because their values come from the key, not from
# the file.  Routing has no claim file; its wrong-key case is in route_verify.
source "helpers.tcl"

foreach id { 5 6 7 8 9 392 393 1102 1103 1104 } {
  suppress_message DPL $id
}

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]
set_wire_rc -clock -layer metal5

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
# One bit away, so the refusal cannot be blamed on a malformed key.
set other 0011223344556677889900aabbccddeeff00112233445566778899aabbccddef

set place_claims [make_result_file wrong_key_place.csv]
place_watermark -key_hex $key -claims_file $place_claims \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64
check "the key that embedded the placement mark verifies it" {
  expr { [wmk::verify_placement_watermark_cmd $key $place_claims] >= 0.75 }
} 1
check "a key one bit away is refused" {
  catch {
    verify_watermark -placement_claims $place_claims -placement_key_hex $other \
      -min_stages 1
  } message
} 1
check "and the refusal says the file was not produced with that key" {
  string match {*WMK-0121*} $message
} 1

# What a forger can do without the key: list pairs and record the order the
# design already shows.  Every such bit is a coin flip against the key.
proc forged_claims { path count } {
  set stream [open $path w]
  puts $stream "kind,A_name,B_name,target_bit,skipped_reason"
  set instances [[ord::get_db_block] getInsts]
  for { set i 0 } { $i < $count } { incr i } {
    set a [lindex $instances [expr { 2 * $i }]]
    set b [lindex $instances [expr { 2 * $i + 1 }]]
    set bit [expr { [[$a getBBox] xMin] < [[$b getBBox] xMin] ? 0 : 1 }]
    puts $stream "pair,[$a getName],[$b getName],$bit,"
  }
  close $stream
}
set forged [make_result_file wrong_key_forged.csv]
forged_claims $forged 40
check "a claim file written to match the design is refused" {
  catch {
    verify_watermark -placement_claims $forged -placement_key_hex $key \
      -min_stages 1
  } message
} 1
check "for the same reason" { string match {*WMK-0121*} $message } 1

clock_tree_synthesis -buf_list CLKBUF_X3 -root_buf CLKBUF_X3 -sink_clustering_enable
set_propagated_clock [all_clocks]
estimate_parasitics -placement
set cts_claims [make_result_file wrong_key_cts.csv]
set committed [cts_watermark -key_hex $key -claims_file $cts_claims]
check "the clock tree yields claims to check" { expr { $committed > 0 } } 1
lassign [wmk::verify_cts_claims_cmd $key $cts_claims] count held probability
check "the key that embedded the clock-tree mark reads its claims" \
  { set count } $committed
check "a key one bit away is refused for the clock tree too" {
  catch {
    verify_watermark -cts_claims $cts_claims -cts_key_hex $other -min_stages 1
  } message
} 1
check "and says so" { string match {*WMK-0122*} $message } 1

exit_summary

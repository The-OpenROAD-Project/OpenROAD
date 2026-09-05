# Which cells carry the mark has to depend on the key.
#
# The target order of a marked pair has always been keyed.  Which pairs get
# marked is the other half, and it is the half that is easy to leave out: an
# embedder that picked candidates by position would still verify perfectly
# against its own claims, and every other test here would still pass.  What it
# would give up is that an observer who knows the algorithm -- which is the
# whole threat model -- could list the marked pairs exactly, and would only
# have to guess their bits.
#
# So this embeds twice with keys that differ in one bit and compares the pairs
# chosen.  One pair per tile is marked rather than the default four, because
# with a quota that small the choice is almost entirely the key's: gcd offers
# 68 eligible pairs and the greedy pass takes the keyed-first one in each tile.
# The counts are exact, not sampled -- both runs are fully determined.
source "helpers.tcl"

suppress_message DPL 5
suppress_message DPL 6
suppress_message DPL 7
suppress_message DPL 8
suppress_message DPL 9
suppress_message DPL 392
suppress_message DPL 393
suppress_message DPL 1102
suppress_message DPL 1103
suppress_message DPL 1104

read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]

set key_a 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set key_b 0011223344556677889900aabbccddeeff00112233445566778899aabbccddef

proc pairs_in { path } {
  set f [open $path r]
  set rows [split [string trim [read $f]] "\n"]
  close $f
  set out {}
  foreach row [lrange $rows 1 end] {
    lappend out [lindex [split $row ,] 1]
  }
  return [lsort $out]
}

set claims_a [make_result_file place_keyed_a.csv]
set claims_b [make_result_file place_keyed_b.csv]
place_watermark -key_hex $key_a -claims_file $claims_a \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 1
place_watermark -key_hex $key_b -claims_file $claims_b \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 1

set a [pairs_in $claims_a]
set b [pairs_in $claims_b]
set shared {}
foreach p $a {
  if { [lsearch -exact -sorted $b $p] >= 0 } { lappend shared $p }
}
puts "key A marked [llength $a], key B marked [llength $b], shared [llength $shared]"

check "both keys find the same amount to mark" \
  { expr { [llength $a] == 5 && [llength $b] == 5 } } 1
check "but they do not mark the same pairs" { llength $shared } 1

exit_summary

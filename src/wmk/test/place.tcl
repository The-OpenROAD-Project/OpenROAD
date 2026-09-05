# The placement watermark round-trips: what the embedder commits is what
# verification finds.
#
# The mark is an ordering, so it only means anything once the design is on
# rows -- gcd_placed.def is gcd.def after detailed_placement, checked in so
# this test measures the watermark rather than the placer.
#
# gcd fills 4.5% of its core, so same-row same-width neighbours are far apart
# and almost no swap is wirelength-neutral at the default gates.  They are
# widened here to give the design any capacity at all; a real design does not
# need that (jpeg commits its per-tile quota at the defaults).  What is being
# tested is the mark, not the gate values.
#
# Twenty-four pairs is the strict pass finding twenty and the capacity fallback
# adding four more, which is the fallback doing its job on a design that cannot
# reach the sixty-four it would like.
#
# place_unmarked.tcl is the other half: it checks these same claims against
# the design as it was before embedding, where they must not hold.
source "helpers.tcl"

# The embedder legalizes after swapping.  Whether that report changes is the
# legalizer's business, not the watermark's.
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

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee
set claims [make_result_file place.csv]

set committed [place_watermark -key_hex $key -claims_file $claims \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64]
puts "committed $committed pairs"
# Pinned rather than merely non-zero: capacity quietly collapsing to a handful
# of pairs would leave a design with too few bits to prove anything, and every
# other check here would still pass.
check "the design yields its usual capacity" { set committed } 24
check "every committed pair holds" { verify_watermark -placement_claims $claims -min_stages 1 } 1

# Some of those pairs have to be ones the embedder actually reordered.  A pair
# that was already in the keyed order is a legitimate claim but a free one, and
# an embedder that claimed nothing else would have written the design's own
# coincidences back out as evidence -- verifying perfectly against a design it
# never touched.  The claim file records which pairs needed a swap, so count
# them.
proc count_swapped { path } {
  set f [open $path r]
  set rows [split [string trim [read $f]] "\n"]
  close $f
  set n 0
  foreach row [lrange $rows 1 end] {
    if { [lindex [split $row ,] 5] eq "" } { incr n }
  }
  return $n
}
puts "swapped [count_swapped $claims] of $committed pairs"
check "and some of them were pairs the embedder had to reorder" \
  { count_swapped $claims } 14

# Embedding the same key twice must not damage the first mark.  The pairs the
# second run picks need not be the same ones -- pairing follows the cells along
# the row, and the first run moved some of them -- but everything it does is
# already in this key's order, so nothing it touches can fall out of it.
place_watermark -key_hex $key -claims_file [make_result_file place_again.csv] \
  -hpwl_eps_um 1.0 -pair_dist_um 3.0 -pairs_per_tile 64
check "re-embedding the same key leaves the first mark intact" \
  { verify_watermark -placement_claims $claims -min_stages 1 } 1

exit_summary

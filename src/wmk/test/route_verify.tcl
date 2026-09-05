# The routing watermark survives the round trip from router to verdict.
#
# bias.tcl proves the cost multiplier reaches the maze router.  This proves the
# rest of the chain: that a keyed half of the nets, routed under that
# multiplier, is afterwards recoverable from the key alone and recognised as a
# watermark.  Nothing from embed time is carried across -- the marked set is
# re-derived from the key -- so a disagreement anywhere between the two ends
# shows up here as a design that fails to prove its own ownership.
#
# The design is routed in full rather than read back from a routed DEF because
# the wrong-way wirelength the statistic measures is the router's output, and a
# checked-in DEF would freeze it.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_def gcd_placed.def
create_clock -name core_clock -period 2.0 [get_ports clk]

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee

set_routing_layers -signal metal1-metal10
set_routing_watermark_strength 100
check "the key tags its half of the nets" \
  { set_routing_watermark -key_hex $key -fraction 0.5 } 197
global_route
detailed_route -verbose 0

check "the marked nets prove ownership" \
  { verify_watermark -routing_key_hex $key -routing_fraction 0.5 -min_stages 1 } 1

# The tags name the marked nets in plaintext, so a design is shipped with them
# dropped.  Verification has to keep working without them: it re-derives the
# marked set from the key, and must not be reading the answer back out of the
# design.  A flow that clears the tags before handing the design over relies on
# this.
check "the tags can be dropped" { clear_routing_watermark } 197
check "and ownership still proves without them" \
  { verify_watermark -routing_key_hex $key -routing_fraction 0.5 -min_stages 1 } 1

exit_summary

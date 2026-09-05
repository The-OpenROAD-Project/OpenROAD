# Companion to bias.tcl: the same route with the multiplier neutral.
# The watermark has to change what the router actually does.
#
# Selection and reporting can all be correct while the grid-cost hook no
# longer reaches the maze router, in which case the watermark is inert and
# every other test still passes.  This routes one design twice, identically
# except for the wrong-way cost multiplier, and records the resulting
# wrong-way wirelength.  With the multiplier neutral the router uses some
# wrong-way wiring; with it raised, it uses none.  If the two ever agree,
# the multiplier is not reaching FlexGridGraph::getCosts.
#
# Every net is tagged, so this measures the multiplier alone and does not
# depend on which nets a particular key happens to select -- select.tcl
# covers that.  The design is deliberately tiny: this is a mechanism guard,
# not a statistical test.
source "helpers.tcl"

set key 0011223344556677889900aabbccddeeff00112233445566778899aabbccddee

proc route_with_strength { strength key } {
  read_lef "sky130hd/sky130hd.tlef"
  read_lef "sky130hd/sky130hd_std_cell.lef"
  read_def "gcd_sky130hd.def"
  read_guides "gcd_sky130hd.guide"
  set_routing_layers -signal met1-met5
  set_routing_watermark_strength $strength
  set_routing_watermark -key_hex $key -fraction 1.0
  detailed_route -verbose 0
  report_routing_watermark
}

route_with_strength 1 $key

# Test that global routing creates a via between co-located pins on
# different layers within the same gcell.
#
# The net has six pins in an H topology: four metal1 BUF_X1 loads at
# the corners form two horizontal bars (top and bottom), connected by
# a vertical spine through the center where a metal4 pin and a metal3
# pin sit adjacent in the same gcell.  FLUTE creates zero-length
# Steiner edges between the co-located center pins; without the
# fillVIA fix for stackAlias resolution these edges are skipped, the
# via between metal3 and metal4 is never created, and RSZ later fails
# with RSZ-0074.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_lef "colocated_pins.lef"
read_def "colocated_pins.def"

set_routing_layers -signal metal2-metal10

set guide_file [make_result_file colocated_pins.guide]

global_route -verbose

write_guides $guide_file

diff_file colocated_pins.guideok $guide_file

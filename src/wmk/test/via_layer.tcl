# A via changes the layer the wire continues on, and that layer decides whether
# what follows is wrong-way.
#
# The decoder reports the change, choosing the via's top or bottom layer by the
# direction of travel, but a reader that ignores it keeps scoring against the
# layer before the via.  Adjacent layers alternate preferred direction, so that
# does not merely lose a little length -- it inverts the classification of every
# run after the first via.
#
# OpenROAD's own writers happen to emit a fresh path after each via, so nothing
# in the rest of the suite exercises this.  The wire below is built by hand in
# the form the encoder also supports: a via followed directly by points, with no
# new path between them.
#
# metal1 prefers horizontal and metal2 prefers vertical, so the run before the
# via is preferred and the run after it is wrong-way.  Read against the wrong
# layer both would count as preferred, and the wrong-way total would be zero.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set block [[[ord::get_db] getChip] getBlock]
set tech [[ord::get_db] getTech]

set metal1 [$tech findLayer metal1]
set via1 [$tech findVia via1_0]

set net [odb::dbNet_create $block "wm_via_probe"]
set wire [odb::dbWire_create $net]
set encoder [odb::dbWireEncoder]
$encoder begin $wire
$encoder newPath $metal1 "ROUTED"
$encoder addPoint 0 0
# 1000 DBU along metal1: horizontal on a horizontal layer, so preferred.
$encoder addPoint 1000 0
# The via moves the wire to metal2.  No new path follows it.
$encoder addTechVia $via1
# 2000 DBU still horizontal, but now on a vertical layer, so wrong-way.
$encoder addPoint 3000 0
$encoder end

# Expect 3000 DBU total and 2000 of it wrong-way.  A reader that ignored the
# via would report 3000 total and 0 wrong-way.
report_routing_watermark -p 0.4

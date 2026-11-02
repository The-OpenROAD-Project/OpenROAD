# Track assignment on a layer whose pitch exceeds the GCell size: some
# GCells hold no metal4 track, so TA must use the nearest tracks outside
# the GCell instead of failing with DRT-0406.
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_def gcd_nangate45_preroute.def
read_guides gcd_nangate45.route_guide

# Replace the vertical metal4 tracks with a 6000 pitch, wider than the
# 4200 GCell, e.g. no track falls in GCell column [12600, 16800).
set block [ord::get_db_block]
set metal4 [[ord::get_db_tech] findLayer metal4]
odb::dbTrackGrid_destroy [$block findTrackGrid $metal4]
set grid [odb::dbTrackGrid_create $block $metal4]
$grid addGridPatternX 190 34 6000
$grid addGridPatternY 140 360 560

detailed_route -verbose 0

puts "drvs: [detailed_route_num_drvs]"

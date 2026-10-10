# DEF requires integers for row, track and gcell grid values. Fractional values
# in a malformed file are warned about and truncated.
source "helpers.tcl"

read_lef "Nangate45/Nangate45.lef"
read_def "read_def_non_integer.def"

set block [ord::get_db_block]
foreach row [$block getRows] {
  puts "[$row getName] origin [$row getOrigin] sites [$row getSiteCount]\
    spacing [$row getSpacing]"
}
foreach grid [$block getTrackGrids] {
  set xs [$grid getGridX]
  set ys [$grid getGridY]
  puts "tracks x [lrange $xs 0 2] ... count [llength $xs]"
  puts "tracks y [lrange $ys 0 2] ... count [llength $ys]"
}

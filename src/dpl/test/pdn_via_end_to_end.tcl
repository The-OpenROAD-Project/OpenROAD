# Fixed supply via metal follows LEF58 end-of-line spacing as drt reads it:
# when both facing edges are line ends, ENDTOEND space replaces the EOL space
# and applies only where the ends overlap.  Every cell's metal2 line end faces
# a via landing above it.
source "helpers.tcl"
read_lef pdn_via_end_to_end.lef
read_def pdn_via_end_to_end.def

detailed_placement -pdn_aware
check_placement

set block [ord::get_db_block]
# Narrow landing, aligned, 80 DBU apart: >= 60 end to end, < 100 EOL.
check "line ends beyond end-to-end space stay" {
  [$block findInst ends_apart] getLocation
} "2000 0"
# Narrow landing, aligned, 40 DBU apart: < 60 end to end.
check "line ends within end-to-end space move" {
  expr {[lindex [[$block findInst ends_close] getLocation] 0] != 4000}
} "1"
# Wide landing is no line end, so 80 DBU < 100 EOL space still conflicts.
check "line end facing a wide shape keeps EOL space" {
  expr {[lindex [[$block findInst end_to_wide] getLocation] 0] != 6000}
} "1"
# Narrow landing 10 DBU to the side, 40 DBU above: within WITHIN, but the
# ends do not overlap, so end to end does not apply.
check "offset line ends that do not overlap stay" {
  [$block findInst ends_offset] getLocation
} "8000 0"
exit_summary

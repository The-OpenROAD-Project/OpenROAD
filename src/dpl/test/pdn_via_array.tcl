# Repeated supply-via array: exercise finite extent, an intentional hole, and
# repeated placement in both row orientations.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef pdn_via_array.lef
read_def pdn_via_array.def

detailed_placement -pdn_aware
check_placement

set block [ord::get_db_block]

# The repeated vias block the two neighboring sites, leaving a deterministic
# nearest legal site on each side of the array.
check "left array conflict location" {
  [$block findInst blocked_left] getLocation
} "23180 5600"
check "left array conflict orientation" {
  [$block findInst blocked_left] getOrient
} R0
check "right array conflict location" {
  [$block findInst blocked_right] getLocation
} "24700 5600"
check "right array conflict orientation" {
  [$block findInst blocked_right] getOrient
} R0

# Site 63 is deliberately absent from the finite array even though its
# neighbors have supply vias.
check "array hole location" {
  [$block findInst gap_boundary] getLocation
} "23940 5600"
check "array hole orientation" {
  [$block findInst gap_boundary] getOrient
} R0

# The site after the finite repetition must not inherit or wrap the pattern.
check "finite array extent location" {
  [$block findInst after_repeat] getLocation
} "25840 5600"
check "finite array extent orientation" {
  [$block findInst after_repeat] getOrient
} R0

# A supply pin on the same net is exempt, including in an FS-oriented row.  An
# unconnected copy of the same mirrored master is not.
check "same-net location" {
  [$block findInst same_net] getLocation
} "25080 8400"
check "same-net orientation" {
  [$block findInst same_net] getOrient
} MX
check "mirrored conflict location" {
  [$block findInst mirrored_blocked] getLocation
} "23940 8400"
check "mirrored conflict orientation" {
  [$block findInst mirrored_blocked] getOrient
} MX

exit_summary

# Test that place_macro ignores the macro being moved during overlap checks.
source "helpers.tcl"

read_lef "./Nangate45/Nangate45.lef"
read_lef "./testcases/place_macro_right_angle_rotation.lef"

read_def "./testcases/place_macro_already_placed_overlap.def"

set block [ord::get_db_block]
set macro [$block findInst macro]
check "target starts placed" { $macro isPlaced } 1

place_macro -macro_name macro -location {40 40} -exact
check "target moves to the requested location" { $macro getLocation } {80000 80000}

foreach {name location} {
  other_left {200 20}
  other_right {400 20}
} {
  check "reject overlap with $name" {
    catch { place_macro -macro_name macro -location $location -exact } error_code
  } 1
  check "report overlap with $name" { set error_code } MPL-0041
}
puts "Other placed macros still reject overlaps."

source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def incr_pin_access.def
set block [ord::get_db_block]
set_debug_level DRT incr_pin_access 1

pin_access -verbose 0
set inst [$block findInst "inst5333"]
$inst setLocation 96400 78660
set inst [$block findInst "inst6286"]
$inst setOrient MX
$inst setLocation 96000 75240
$block updatePinAccess

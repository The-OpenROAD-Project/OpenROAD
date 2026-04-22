# Exercises SwapPinsMove/SizeUpMove/UnbufferMove through repair_timing -sequence.
# Regression guard for issue #10210: the three moves resolve the driver's
# input LibertyPort via prev_arc->from() (stable) rather than
# prevPath()->pin() (which could point into reallocated Vertex::paths_).
source "helpers.tcl"
if { ![info exists repair_args] } {
  set repair_args {}
}
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_sizedown.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement
report_checks -fields input -digits 3

set_dont_use [get_lib_cells CLKBUF*]
repair_timing -setup -sequence "swap,sizeup,unbuffer" {*}$repair_args
report_checks -fields input -digits 3

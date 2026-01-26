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

write_verilog_for_eqy repair_setup_sizedown before "None"
set_dont_use [get_lib_cells CLKBUF*]
repair_timing -setup -sequence "sizedown" {*}$repair_args
run_equivalence_test repair_setup_sizedown \
  -lib_dir ./Nangate45/work_around_yosys/ \
  -remove_cells "None"
report_checks -fields input -digits 3

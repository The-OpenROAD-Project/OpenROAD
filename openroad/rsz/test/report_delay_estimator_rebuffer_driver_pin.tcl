# -driver_pin must be an output pin of -inst: a pin of another instance and
# an input pin of -inst are both rejected.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_minimal.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

catch {
  report_delay_estimator_accuracy_for_rebuffering -inst r2 -driver_pin r1/Q
} error
puts $error

catch {
  report_delay_estimator_accuracy_for_rebuffering -inst r1 -driver_pin r1/D
} error
puts $error

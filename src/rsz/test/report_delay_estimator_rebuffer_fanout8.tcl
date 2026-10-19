# High-fanout single junction (8 sinks).
# Tests the tree plan and the side-load estimates of the seven sinks besides
# the target.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_fanout8.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "fanout8: 8-sink single-junction radial"
report_delay_estimator_accuracy_for_rebuffering \
  -inst r1 \
  -driver_pin r1/Q

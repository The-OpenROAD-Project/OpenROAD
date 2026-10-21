# Multi-level Steiner (4 sinks in 2 distant clusters).
# Tests nested junctions and recursive BufferedNet::cap() propagation.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_steiner.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "steiner: 2 distant clusters of 2 sinks each"
report_delay_estimator_accuracy_for_rebuffering \
  -inst r1 \
  -driver_pin r1/Q

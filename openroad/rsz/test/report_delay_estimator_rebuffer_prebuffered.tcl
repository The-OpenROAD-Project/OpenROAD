# Pre-buffered fanout: side_u1 and side_u2 already drive two of the sinks.
# Tests that the plan keeps the existing buffers as tree loads, so the r5/D
# path targets side_u2/A.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_prebuffered.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

puts "prebuffered: existing 2-buffer trunk + 3-sink fanout"
report_delay_estimator_accuracy_for_rebuffering \
  -inst r1 \
  -driver_pin r1/Q

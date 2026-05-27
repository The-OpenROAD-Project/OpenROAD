# Regression for the unbounded buffer-chain bug in
# RepairDesign::repairNetWire.
#
# When the slew quadratic has no positive root -- i.e. when
#   r_drvr * ref_cap >= max_load_slew_margined / slew_rc_factor_
# -- split_length collapses to 0 and the next repeater would be placed
# at the same coordinate as the current load.  Before the fix the outer
# while-loop iterated unboundedly, inserting thousands of buffers at the
# same point until Levelize::visit recursion overflowed the stack.
#
# Force the condition with a 6 ps max_transition on Nangate45.  After a
# few splits the remaining-wire quadratic admits no positive root, so
# split_length goes to 0.  With the fix repair_design emits RSZ-0170 and
# exits cleanly.  Without the fix the test hangs in repairNetWire.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_slew10.def
set_max_transition 0.006 [current_design]

set_wire_rc -resistance 3.574e-02 -capacitance 7.516e-02
estimate_parasitics -placement

repair_design
puts "repair_design completed"

# repair_timing checks -effort and -profile_mode in C++, so every command that
# reaches repairSetup() reports the same errors.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def report_delay_estimator_rebuffer_minimal.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

catch { repair_timing -setup -phases PARALLEL_EVAL -effort max } error
puts $error

catch { repair_timing -setup -profile_mode 4 } error
puts $error

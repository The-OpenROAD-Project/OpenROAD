# PARALLEL_EVAL takes its reference clock period from the clocks of every
# mode, not only the command mode.  The func mode clocks core_clock at
# 0.55 ns and the test mode at 0.45 ns, so the reference period is 0.45 ns
# while func stays the command mode.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45_placed.def

set lib [get_full_name [lindex [get_libs *] 0]]
foreach {mode period} {func 0.55 test 0.45} {
  set_mode $mode
  create_clock -name core_clock -period $period [get_ports clk]
  set_input_delay -clock core_clock 0.05 [all_inputs -no_clocks]
  set_output_delay -clock core_clock 0.1 [all_outputs]
  define_scene scene_$mode -mode $mode -liberty $lib
}
set_mode func

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# The debug lines print the reference clock period and where the WNS tail
# starts.
set_debug_level RSZ repair_setup 1
repair_timing -setup -phases PARALLEL_EVAL
set_debug_level RSZ repair_setup 0

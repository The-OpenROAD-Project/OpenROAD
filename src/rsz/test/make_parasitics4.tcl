# 2 corners with wire parasitics
define_corners ss ff
read_liberty -corner ss Nangate45/Nangate45_slow.lib
read_liberty -corner ff Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def reg6.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

set_wire_rc -layer metal1
set corner [sta::find_corner ss]
# ohm/meter -> kohm/micron
set r [expr [rsz::wire_signal_resistance $corner] * 1e-3 * 1e-6]
# F/meter -> fF/micron
set c [expr [rsz::wire_signal_capacitance $corner] * 1e+15 * 1e-6]
set_wire_rc -corner ff -resistance [expr $r * 0.8] -capacitance [expr $c * 0.8]
set_wire_rc -corner ss -resistance [expr $r * 1.2] -capacitance [expr $c * 1.2]
estimate_parasitics -placement

report_net -connections -verbose r1/Q -corner ff
report_net -connections -verbose r1/Q -corner ss

report_checks -corner ff -path_delay min -fields capacitance
report_checks -corner ss -path_delay max -fields capacitance


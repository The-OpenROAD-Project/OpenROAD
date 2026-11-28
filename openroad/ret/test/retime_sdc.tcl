# What the SDC names stays in place: a move across a gate or a register
# that a constraint names would change the paths it covers.  The same
# design without a constraint (retime_basic) has 2 pinned gates and takes
# 3 slides.  A disabled arc of g2 or case analysis on its input pins g2 (3
# pinned gates, not 2), and a disabled arc of the library cell pins every
# inverter (6 pinned gates): no path through them is timed then, so the
# count is what shows it.  r1 stays, so g1 is pinned (3 pinned gates),
# with a disabled wire edge into it, which is still disabled after retime
# (OpenSTA drops such an entry when either pin is disconnected), with a
# false path to its D pin or that pin disabled, a derate on the cell, a
# port delay measured from its clock pin, a path delay that names it as a
# cell, whose requirement stays on the path, and a path margin from it,
# which -move_sdc_registers does not release.  With a margin through g2
# the register slides across g1 only (1 slide) and the margin stays on the
# path.  A hold multicycle through g2 then keeps the register in front of
# g2, on a path that still fails setup.  A path delay through g2 names no
# register, so -move_sdc_registers leaves g2 pinned and nothing more moves.
# A path delay and a path group on every register ([all_registers]) each
# keep the registers in place, until -move_sdc_registers releases both:
# the last retime moves the register.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

proc retime_case { name args } {
  puts [string trim "$name $args"]
  retime {*}$args
}

set_disable_timing [get_cells g2] -from A -to ZN
retime_case "disabled arc of g2"
unset_disable_timing [get_cells g2] -from A -to ZN

set_case_analysis 1 [get_pins g2/A]
retime_case "case analysis on g2/A"
unset_case_analysis [get_pins g2/A]

set inv [get_lib_cells NangateOpenCellLibrary/INV_X1]
set_disable_timing $inv -from A -to ZN
retime_case "disabled arc of INV_X1"
unset_disable_timing $inv -from A -to ZN

set edge [get_timing_edges -from [get_pins g0/ZN] -to [get_pins r1/D]]
set_disable_timing $edge
retime_case "disabled wire edge into r1"
report_disabled_edges
unset_disable_timing $edge

set_false_path -to [get_pins r1/D]
retime_case "false path to r1/D"
unset_path_exceptions -to [get_pins r1/D]

set_disable_timing [get_pins r1/D]
retime_case "disabled pin r1/D"
unset_disable_timing [get_pins r1/D]

set_timing_derate -late 1.05 [get_cells r1]
retime_case "derate on r1"
unset_timing_derate

set_input_delay 0.01 -clock clk -reference_pin [get_pins r1/CK] [get_ports a]
retime_case "input delay from r1/CK"
unset_input_delay -clock clk [get_ports a]

set_max_delay 0.05 -from [get_cells r1] -to [get_cells r2]
retime_case "path delay from r1 to r2"
unset_path_exceptions -from [get_cells r1] -to [get_cells r2]

set_path_margin -setup 0.02 -from [get_cells r1] -to [get_cells r2]
retime_case "path margin from r1 to r2" -move_sdc_registers
unset_path_exceptions -from [get_cells r1] -to [get_cells r2]

set_path_margin -setup 0.02 -through [get_pins g2/ZN]
retime_case "path margin through g2"
report_checks -through [get_pins g2/ZN] -format end -digits 4
unset_path_exceptions -through [get_pins g2/ZN]

set_multicycle_path 1 -hold -through [get_pins g2/ZN]
retime_case "hold multicycle through g2"
unset_path_exceptions -through [get_pins g2/ZN]

set_max_delay 0.05 -through [get_pins g2/ZN]
retime_case "path delay through g2" -move_sdc_registers
report_checks -through [get_pins g2/ZN] -format end -digits 4
unset_path_exceptions -through [get_pins g2/ZN]

set_max_delay 0.2 -from [all_inputs] -to [all_registers]
retime_case "path delay to all registers"
unset_path_exceptions -from [all_inputs] -to [all_registers]

group_path -name registers -to [all_registers]
retime_case "path group of all registers"
set_max_delay 0.2 -from [all_inputs] -to [all_registers]
retime_case "path delay and path group" -move_sdc_registers

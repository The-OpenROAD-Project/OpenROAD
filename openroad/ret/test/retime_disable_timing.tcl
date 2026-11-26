# A cell with a timing edge that set_disable_timing disables stays, and so
# does a gate with case analysis on a pin: a slide across the gate would
# move the paths that end in front of it onto an arc that is not timed.  g2
# is pinned by a disabled arc and by a constant on its input (3 pinned
# gates, not 2), and every inverter by a disabled arc of the library cell
# (6 pinned gates).  Then a disabled wire edge into r1 keeps r1 in place
# (OpenSTA drops such an entry when either pin is disconnected), and the
# edge is still disabled after retime.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk
set_input_delay 0 -clock clk [get_ports {a b}]

set_disable_timing [get_cells g2] -from A -to ZN
retime
unset_disable_timing [get_cells g2] -from A -to ZN

set_case_analysis 1 [get_pins g2/A]
retime
unset_case_analysis [get_pins g2/A]

set_disable_timing [get_lib_cells NangateOpenCellLibrary/INV_X1] -from A -to ZN
retime
unset_disable_timing [get_lib_cells NangateOpenCellLibrary/INV_X1] -from A -to ZN

set_disable_timing [get_timing_edges -from [get_pins g0/ZN] -to [get_pins r1/D]]
retime
report_disabled_edges

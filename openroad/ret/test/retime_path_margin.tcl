# A cell that an exception names stays, whatever the kind of exception: a
# move across it would change which paths the exception covers.  A path
# margin from r1 to r2 keeps r1 in place, and -move_named_registers does not
# release it: the option is for path delays and path groups.  With a margin
# through g2 the register slides across g1 only (3 pinned gates, 1 slide) and
# the margin stays on the path.  A path delay through g2 names no register,
# so -move_named_registers leaves g2 pinned and nothing more moves.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.1 clk

set_path_margin -setup 0.02 -from [get_cells r1] -to [get_cells r2]
retime -move_named_registers
unset_path_exceptions -from [get_cells r1] -to [get_cells r2]

set_path_margin -setup 0.02 -through [get_pins g2/ZN]
retime
report_checks -through [get_pins g2/ZN] -format end -digits 4
unset_path_exceptions -through [get_pins g2/ZN]

set_max_delay 0.05 -through [get_pins g2/ZN]
retime -move_named_registers
report_checks -through [get_pins g2/ZN] -format end -digits 4
report_worst_slack -max -digits 4

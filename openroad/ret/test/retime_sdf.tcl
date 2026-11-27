# A design with delays annotated from SDF is not retimed.  STA drops an
# annotated wire delay with the wire, and a trial that is undone brings the
# wire back without it.  The hold uncertainty makes STA reject every trial.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.13 clk
set_clock_uncertainty -hold 0.30 clk
read_sdf retime_sdf.sdf

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

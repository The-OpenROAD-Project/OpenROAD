# A design with parasitics read from SPEF is not retimed.  STA takes a
# disconnected pin off its SPEF network and the estimator cannot rebuild it,
# so a trial that is undone would lose the wire.  The hold uncertainty makes
# STA reject every trial.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.13 clk
set_clock_uncertainty -hold 0.30 clk
read_spef retime_spef.spef

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

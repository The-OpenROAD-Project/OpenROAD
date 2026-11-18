# repair_clock_inverters with a clock inverter inside a submodule: in the
# hierarchical flow the inverter's output pin resolves to a dbModNet, and
# deleting the inverter after cloning must not crash in deleteNet.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_clk_inverters2_hier.v
link_design -hier top
create_clock -period 1 clk

repair_clock_inverters

report_net clk
report_net c1_1/ZN
report_net c1_2/ZN
report_net c1_3/ZN

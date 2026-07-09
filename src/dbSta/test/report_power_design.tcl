# report_power design-level grouped summary (additive, deterministic).
# Reuses the power1.v small clocked design (sequential DFFs + combinational
# logic) and pins switching activity so the grouped Internal/Switching/
# Leakage/Total summary is stable across runs.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog power1.v
link_design top
create_clock -name clk -period 10 {clk1 clk2 clk3}
set_input_delay -clock clk 0 {in1 in2}

# Deterministic activity: fixed global + input switching activity.
set_power_activity -global -activity 0.1
set_power_activity -input -activity 0.1

# Design-level grouped power summary
# (Sequential / Combinational / Clock / Macro / Pad / Total rows,
#  Internal / Switching / Leakage / Total columns).
report_power -digits 3

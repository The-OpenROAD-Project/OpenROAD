# Buffers inserted by repair_design must be clamped inside the core area.
# Tight core (10-90 um within a 200x200 die) plus loads at die corners forces
# Steiner points outside the core, exercising clampLocToCore on both fanout
# and wire repeater paths. .ok captures the per-buffer clamp debug log.
source "helpers.tcl"
source Nangate45/Nangate45.vars

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog repair_design_outside_core.v
link_design outside_core
read_def -floorplan_initialize repair_design_outside_core.def

create_clock -period 10 clk
source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

set_debug_level RSZ buffer_clamp 1
set_max_fanout 5 [current_design]
repair_design -max_wire_length 100

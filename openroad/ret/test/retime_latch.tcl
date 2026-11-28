# A transparent latch in the design is don't-touch, not an error.  The latch
# is fed by a gate, so it is on the retiming graph (4 registers), and the
# worst path starts at it; it stays, and the gate behind it is never crossed
# (5 pinned gates, not 4).  The register chain next to it is still retimed.
source "helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_latch.v
link_design top
create_clock -period 0.1 clk

retime

report_new_cells

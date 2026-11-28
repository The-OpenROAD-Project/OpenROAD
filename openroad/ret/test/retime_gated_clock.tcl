# One clock with a gated copy: retime runs, and each new register is on the
# clock net of the register it replaces, the gated one for the gated chains,
# also when it moves more than one gate deep and when that register is
# removed before its replacement is made.  A gate fed from both clock nets
# is pinned, and so are the gates fed from a net whose registers are on two
# clock nets.
source "helpers.tcl"
source "ret_report.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_gated_clock.v
link_design top
create_clock -period 0.1 clk

retime

report_new_cells

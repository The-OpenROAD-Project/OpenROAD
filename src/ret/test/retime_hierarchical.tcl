# A register chain inside a submodule that is fed from the parent module:
# the first register's input comes through the module port, so a move would
# take it across the module boundary, and nothing slides.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_hierarchical.v
link_design -hier top
create_clock -period 1.0 clk

retime
check_axioms

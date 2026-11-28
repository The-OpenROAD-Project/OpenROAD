# A shift register of scan flip-flops the library does not mark as scan
# cells, each reading the one before it on both D and SI.  The graph walk
# enters a register once, so the chain is one edge; entering it by each pin
# would make 2^24 of them.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_scan_chain.v
link_design top
create_clock -period 0.5 clk

retime

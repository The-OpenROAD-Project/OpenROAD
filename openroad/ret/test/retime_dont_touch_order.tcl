# A fence holds whatever the order of the gates in the block.  Here the
# driver h1 comes after g2.  With the net behind g2 marked don't-touch, or
# g2 itself, h1 is pinned (4 pinned gates) and nothing moves.  The last
# retime, with no fence, moves the register.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_dont_touch_order.v
link_design top
create_clock -period 0.1 clk

set_dont_touch n2
retime
unset_dont_touch n2

set_dont_touch g2
retime
unset_dont_touch g2

retime

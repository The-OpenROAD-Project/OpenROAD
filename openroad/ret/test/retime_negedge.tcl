# A negative-edge register in the design is an error.
source "helpers.tcl"
read_lef sequnmap_basic.lef
read_lef sequnmap_seq_active_low.lef
read_liberty sequnmap_basic.lib
read_liberty sequnmap_seq_active_low.lib
read_verilog retime_negedge.v
link_design top

catch { retime } msg
puts $msg

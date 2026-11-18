# A library without the cells retime needs (no inverter, AND2, OR2 or
# positive-edge register) is an error.
source "helpers.tcl"
read_lef sequnmap_basic.lef
read_lef sequnmap_seq_active_low.lef
read_liberty sequnmap_seq_active_low.lib
read_verilog retime_no_base_cell.v
link_design top

catch { retime } msg
puts $msg

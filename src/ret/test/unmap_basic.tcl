# Unmap an enable flop and an inverted-output flop; an async-reset flop stays.
source "helpers.tcl"
read_lef sequnmap_basic.lef
read_liberty sequnmap_basic.lib
read_verilog unmap_basic.v
link_design top

unmap_sequential_cells

set v_file [make_result_file unmap_basic.v]
write_verilog $v_file
diff_files unmap_basic.vok $v_file

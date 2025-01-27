source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
#read_lef dummy_pads.lef
read_liberty Nangate45/Nangate45_typ.lib
read_verilog simple_test_hier.v
link test_16_sinks -hier

initialize_floorplan -die_area "0 0 40 1200"   -core_area "0 0 40 1200" -site FreePDK45_38x28_10R_NP_162NW_34O
#make_io_sites -horizontal_site IOSITE -vertical_site IOSITE -corner_site IOSITE -offset 15

source Nangate45/Nangate45.vars
source Nangate45/Nangate45.rc

#place_pad -master PADCELL_SIG_V -row IO_EAST -location 500 "clk"

global_placement -skip_nesterov_place 
detailed_placement

create_clock -period 5 clk

set_wire_rc -clock -layer metal3

clock_tree_synthesis -root_buf CLKBUF_X3  -buf_list CLKBUF_X3   -wire_unit 20  -apply_ndr    

set verilog_file [make_result_file simple_test_hier_out.v]
write_verilog $verilog_file
diff_files simple_test_hier_out.vok $verilog_file

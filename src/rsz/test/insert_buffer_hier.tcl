# insert_buffer command test with hierarchical flow
source "helpers.tcl"

set test_name insert_buffer_hier

define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_verilog rebuffer1.v
link_design -hier top
read_def -floorplan_initialize rebuffer1.def

puts "--- Test 1: After Driver ---"
set net [get_nets u_mid1/u_leaf1/n1]
set buf1 [insert_buffer \
  -net $net \
  -buffer_cell BUF_X1 \
  -buffer_name b_after_drvr \
  -net_name n_after_drvr]
puts "Inserted after driver: [get_name $buf1]"

puts "--- Test 2: Before Single Load ---"
set pin [get_pins u_mid1/u_leaf1/buf2/A]
set buf2 [insert_buffer \
  -load_pins $pin \
  -buffer_cell BUF_X1 \
  -buffer_name b_before_load \
  -net_name n_before_load \
  -location {10.0 20.0}]
puts "Inserted before single load: [get_name $buf2]"

puts "--- Test 3: Before Multiple Loads ---"
set net2 [get_nets u_mid1/l2_out1]
set loads [get_pins {u_mid1/dff_load1/D u_mid1/dff_load2/D}]
set buf3 [insert_buffer \
  -net $net2 \
  -load_pins $loads \
  -buffer_cell BUF_X1 \
  -buffer_name b_before_loads \
  -net_name n_before_loads]
puts "Inserted before multiple loads: [get_name $buf3]"

puts "--- Test 4: Before Multiple Loads (Inferred Net) ---"
set loads2 [get_pins {u_mid1/dff_load3/D u_mid1/dff_load4/D}]
set buf4 [insert_buffer \
  -load_pins $loads2 \
  -buffer_cell BUF_X1 \
  -buffer_name b_inferred \
  -net_name n_inferred]
puts "Inserted with inferred net: [get_name $buf4]"

# generate .v and .def files
set out_verilog [make_result_file "${test_name}.v"]
write_verilog $out_verilog
diff_files "${test_name}.vok" $out_verilog

set out_def [make_result_file "${test_name}.def"]
write_def $out_def
diff_files "${test_name}.defok" $out_def

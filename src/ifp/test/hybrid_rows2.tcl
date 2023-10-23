# init_floorplan
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_verilog hybrid_rows2.v
link_design top
set_debug_level IFP hybrid 1
initialize_floorplan -die_area "0 0 300 300" \
  -core_area "100 100 1000 1000" 

set def_file [make_result_file hybrid_rows2.def]
write_def $def_file
diff_files hybrid_rows2.defok $def_file

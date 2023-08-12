# init_floorplan
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45/fake_macros.lef
read_verilog hybrid_rows.v
link_design top
initialize_floorplan -die_area "0 0 1000 1000" \
  -core_area "100 100 900 900" 

set def_file [make_result_file hybrid_rows.def]
write_def $def_file
diff_files hybrid_rows.defok $def_file

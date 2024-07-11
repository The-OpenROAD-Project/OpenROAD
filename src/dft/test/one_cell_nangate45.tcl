source "helpers.tcl"
set LIB_DIR "./Nangate45"
#
set tech_lef "$LIB_DIR/Nangate45_tech.lef"
set std_cell_lef "$LIB_DIR/Nangate45.lef"
set fake_macro_lef "$LIB_DIR/fakeram45_512x64.lef $LIB_DIR/fakeram45_64x7.lef $LIB_DIR/fakeram45_64x96.lef"
set liberty_file "$LIB_DIR/Nangate45_fast.lib"
set fake_macro_lib  "$LIB_DIR/fakeram45_512x64.lib $LIB_DIR/fakeram45_64x7.lib $LIB_DIR/fakeram45_64x96.lib"
 
set synth_verilog "./testcases/bp_fe_top.v"
set floorplan_def "./testcases/bp_fe_top.def"
set sdc_file "./testcases/bp_fe_top.sdc"
set top_module "bp_fe_top"
 
read_lef $tech_lef
read_lef $std_cell_lef
foreach lef_file $fake_macro_lef {
    puts "read lef file $lef_file"
    read_lef $lef_file

}
read_liberty $liberty_file
foreach lib_file $fake_macro_lib {
    puts "read_lib file $lib_file"
    read_liberty $lib_file
}
 
read_verilog one_cell_nangate45.v
link_design one_cell

scan_replace
insert_dft

set verilog_file [make_result_file one_cell_nandgate45.v]
write_verilog $verilog_file
diff_files $verilog_file one_cell_nandgate45.vok

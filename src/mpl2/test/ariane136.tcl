source "helpers.tcl"
set LIB_DIR "./Nangate45"
#
set tech_lef "$LIB_DIR/Nangate45_tech.lef"
set std_cell_lef "$LIB_DIR/Nangate45.lef"
set fake_macro_lef "$LIB_DIR/fakeram45_256x16.lef"
set liberty_file "$LIB_DIR/Nangate45_fast.lib"
set fake_macro_lib  "$LIB_DIR/fakeram45_256x16.lib"
 
set synth_verilog "./testcases/ariane136/ariane.v"
set floorplan_def "./testcases/ariane136/ariane.def"
set sdc_file "./testcases/ariane136/ariane.sdc"
set top_module "ariane"
 
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
 
read_verilog $synth_verilog
link_design $top_module
read_sdc $sdc_file
#
read_def $floorplan_def -floorplan_initialize 

rtl_macro_placer -max_num_inst 200000 -min_num_inst 5000 \
                 -max_num_macro 140 -min_num_macro 4 \
                 -halo_width 10 -snap_layer 8 \
                 -report_directory results/ariane136

set def_file [make_result_file araine_out.def]
write_def $def_file
diff_files ./testcases/ariane136/ariane.defok $def_file

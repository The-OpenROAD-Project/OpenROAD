source "helpers.tcl"
set LIB_DIR "./Nangate45"
set DESIGN_DIR "/home/ravi/Designs/MP_Test"
#
set tech_lef "$LIB_DIR/Nangate45_tech.lef"
set std_cell_lef "$LIB_DIR/Nangate45.lef"
set fake_macro_lef "$LIB_DIR/fake_macros.lef"
set liberty_file "$LIB_DIR/Nangate45_fast.lib"
 
set synth_verilog "./testcases/mp_test1.v"
set floorplan_def "./testcases/mp_test1_fp.def"
set top_module "mp_test1"
 
read_lef $tech_lef
read_lef $std_cell_lef
read_lef $fake_macro_lef
read_liberty $liberty_file
 
read_verilog $synth_verilog
link_design $top_module
#
read_def $floorplan_def -floorplan_initialize
#
partition_design -max_num_inst 1000 -min_num_inst 200 \
                    -max_num_macro 10 -min_num_macro 5 \
                    -net_threshold 5 -virtual_weight 500  \
                    -report_file partition.txt

rtl_macro_placer -config_file config.txt

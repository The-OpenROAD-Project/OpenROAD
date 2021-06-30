set LIBDIR "./Nangate45"
#
set tech_lef "$LIBDIR/Nangate45_tech.lef"
set std_cell_lef "$LIBDIR/Nangate45.lef"
set liberty_file "$LIBDIR/Nangate45_fast.lib"
 
set synth_verilog "./testcases/aes_cipher_top.v"
set sdc_file "./testcases/aes_cipher_top.sdc"
set floorplan_file "./testcases/aes_fp.def"
set config_file "./testcases/aes_config.txt"
 
set top_module "aes_cipher_top"
 
read_lef $tech_lef
read_lef $std_cell_lef
read_liberty $liberty_file
 
read_verilog $synth_verilog
link_design $top_module

read_def $floorplan_file -floorplan_initialize

partition_design -max_num_inst 1000 -min_num_inst 200 \
                    -max_num_macro 10 -min_num_macro 5 \
                    -net_threshold 5 -virtual_weight 500  \
                    -report_directory results/aes_macro_place \
                    -report_file aes_macro_place

rtl_macro_placer -config_file $config_file

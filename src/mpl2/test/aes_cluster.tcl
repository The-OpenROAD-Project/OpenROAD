set LIBDIR "./library/nangate45"
#
set tech_lef "$LIBDIR/lef/NangateOpenCellLibrary.tech.lef"
set std_cell_lef "$LIBDIR/lef/NangateOpenCellLibrary.macro.lef"
set liberty_file "$LIBDIR/lib/NangateOpenCellLibrary_typical.lib"
 
set site "FreePDK45_38x28_10R_NP_162NW_34O"
 
set synth_verilog "./testcases/aes_cipher_top.v"
set sdc_file "./testcases/aes_cipher_top.sdc"
set floorplan_file "./testcases/aes_fp.def"
 
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
                    -report_file partition.txt

#rtl_macro_placer -config_file config.txt

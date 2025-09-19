# Test for connect by abutment where one pin is connected to a floating net
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_lef Nangate45_io/dummy_pads.lef

read_def Nangate45_blackparrot/floorplan_flipchip.def

set net [odb::dbNet_create [ord::get_db_block] testnet0]
set iterm [[ord::get_db_block] findITerm u_ddr_dqs_p_3_io/SNS]
$iterm connect $net
set iterm [[ord::get_db_block] findITerm u_bsg_tag_clk_i/SNS]
$iterm connect $net

set net [odb::dbNet_create [ord::get_db_block] testnet1]
set iterm [[ord::get_db_block] findITerm u_ddr_dm_3_o/SNS]
$iterm connect $net

connect_by_abutment

set def_file [make_result_file "connect_by_abutment_with_single_pinnet.def"]
write_def $def_file
diff_files $def_file "connect_by_abutment_with_single_pinnet.defok"

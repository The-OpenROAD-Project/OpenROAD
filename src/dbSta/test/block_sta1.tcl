# block sta using design libraries
source "helpers.tcl"
read_lef example1.lef
read_def example1.def
read_liberty example1_typ.lib

set_input_delay 0 in1
create_clock -period 10 {clk1 clk2 clk3}
report_checks

################################################################
# make block sta
# libraries are shared with original sta

set sta [ord::get_sta]
set db [ord::get_db]
set block [[$db getChip] getBlock]
# make block inside design block
set block2 [odb::dbBlock_create $block "block2"]

# make chain of 2 buffers
set buf_master [$db findMaster "BUF_X1"]
set b1 [odb::dbInst_create $block2 $buf_master "b1" NULL]
set b2 [odb::dbInst_create $block2 $buf_master "b2" NULL]

set in1_net [odb::dbNet_create $block2 "in1"]
set in1_bterm [odb::dbBTerm_create $in1_net "in1"]

set b1_A [$b1 findITerm "A"]
odb::dbITerm_connect $b1_A $in1_net

set n1 [odb::dbNet_create $block2 "n1"]
set b1_Z [$b1 findITerm "Z"]
set b2_A [$b2 findITerm "A"]
odb::dbITerm_connect $b1_Z $n1
odb::dbITerm_connect $b2_A $n1

set out1_net [odb::dbNet_create $block2 "out1"]
set out1_bterm [odb::dbBTerm_create $out1_net "out1"]
$out1_bterm setIoType "OUTPUT"
set b2_Z [$b2 findITerm "Z"]
odb::dbITerm_connect $b2_Z $out1_net

set sta2 [sta::make_block_sta $block2]
# switch sta used by commands
ord::set_cmd_sta $sta2

# pi elmore wire model between buffers
sta::set_pi_model b1/Z .1 100 .1
sta::set_elmore b1/Z b2/A .2

# various delay reports
set sta_report_default_digits 4
report_arrival out1
report_edges -from b1/A
report_edges -from b1/Z
report_dcalc -from b1/A

################################################################
# switch back to design sta

ord::set_cmd_sta $sta
report_checks

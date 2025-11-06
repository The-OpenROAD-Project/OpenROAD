# remove_buffers in1 -> b1 -> b2 -> b3 -> out1
source "helpers.tcl"

set test_name remove_buffers5

read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef

read_verilog asap7_riscv32i.v
link_design riscv_top -hier

create_clock [get_ports clk] -name core_clock -period 3000

#source Nangate45/Nangate45.rc
#set_wire_rc -layer metal3

# make sure sta works before/after removal
set sp {riscv.dp.pcreg.q[31]$_DFF_PP0_/CLK}
set ep {riscv.dp.rf.rf[13][17]$_DFFE_PP_/D}

estimate_parasitics -placement
report_checks -from $sp -through _10996_/Y -through _14612_/Y -through dmem/_156_/Y -to $ep -digits 5

remove_buffers

report_checks -from $sp -through _10996_/Y -through _14612_/Y -through dmem/_156_/Y -to $ep -digits 5

estimate_parasitics -placement
report_checks -from $sp -through _10996_/Y -through _14612_/Y -through dmem/_156_/Y -to $ep -digits 5

sta::network_changed
estimate_parasitics -placement
report_checks -from $sp -through _10996_/Y -through _14612_/Y -through dmem/_156_/Y -to $ep -digits 5

#set repaired_filename [make_result_file $test_name.def]
#write_def $repaired_filename
#diff_file $test_name.defok $repaired_filename

# A register in a scan chain of the database, as a DEF SCANCHAINS section
# makes it, stays: its liberty cell does not show it, and odb refuses to
# delete such a cell.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_verilog retime_basic.v
link_design top
create_clock -period 0.13 clk

set block [ord::get_db_block]
set chain [odb::dbScanChain_create [$block getDft]]
set scan_list [odb::dbScanList_create [odb::dbScanPartition_create $chain]]
$scan_list add [$block findInst r1]

report_worst_slack -max -digits 4
retime
report_worst_slack -max -digits 4

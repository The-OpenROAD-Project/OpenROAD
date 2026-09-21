# A false boolean property must not count as an active routing watermark.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def
set net [odb::dbNet_create [ord::get_db_block] report_tag]
set encoder [odb::dbWireEncoder]
$encoder begin [odb::dbWire_create $net]
$encoder newPath [[ord::get_db_tech] findLayer metal1] ROUTED
$encoder addPoint 0 0
$encoder addPoint 1000 0
$encoder end

set property [odb::dbBoolProperty_create $net watermark false]
tee -variable output { report_routing_watermark }
check "false property is not an active tag" \
  { string match {*X=0 watermark nets*} $output } 1
$property setValue true
tee -variable output { report_routing_watermark }
check "true property is an active tag" \
  { string match {*X=1 watermark nets*} $output } 1
clear_routing_watermark
tee -variable output { report_routing_watermark }
check "absent property is not an active tag" \
  { string match {*X=0 watermark nets*} $output } 1
exit_summary

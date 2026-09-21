# Verify that -congestion_report_file is honored when CUGR is the
# routing engine. The file must be written and contain the TR-format
# marker dump that the DRC viewer can re-load.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set rpt_file [make_result_file "congestion_report_file_cugr.rpt"]
set_global_routing_layer_adjustment metal1-metal10 0.9

global_route -use_cugr -verbose -congestion_report_file $rpt_file

check "Congestion report file was written" {
  expr { [file exists $rpt_file] }
} 1

check "Congestion report file is non-empty" {
  expr { [file size $rpt_file] > 0 }
} 1

check "Congestion report file contains TR-format marker entries" {
  set fh [open $rpt_file r]
  set content [read $fh]
  close $fh
  expr { [string first "violation type:" $content] >= 0 }
} 1

exit_summary

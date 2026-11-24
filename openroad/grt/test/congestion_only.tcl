# Verify that global_route -congestion_only runs FastRoute's initial 2D L-route
# congestion estimation, writes TR-format congestion markers without generating
# 3D route guides, clears stale markers when uncongested, and rejects -use_cugr.
source "helpers.tcl"
read_lef "Nangate45/Nangate45.lef"
read_def "gcd.def"

set rpt_file [make_result_file "congestion_only.rpt"]
set_global_routing_layer_adjustment metal1-metal10 0.9

global_route -congestion_only -allow_congestion -verbose \
  -congestion_report_file $rpt_file

check "Congestion is detected in -congestion_only mode" {
  has_congestion
} 1

check "Congestion report file was written and non-empty" {
  expr { [file exists $rpt_file] && [file size $rpt_file] > 0 }
} 1

check "Congestion report file contains TR-format marker entries" {
  set fh [open $rpt_file r]
  set content [read $fh]
  close $fh
  expr { [string first "violation type:" $content] >= 0 }
} 1

set block [ord::get_db_block]
set total_guides 0
foreach net [$block getNets] {
  incr total_guides [llength [$net getGuides]]
}
check "No 3D route guides are generated in -congestion_only mode" {
  expr { $total_guides }
} 0

# Re-run without layer adjustment so congestion clears; report file must be
# truncated and Global route marker category removed.
set_global_routing_layer_adjustment metal1-metal10 0.0
global_route -congestion_only -congestion_report_file $rpt_file

check "Uncongested -congestion_only run clears congestion" {
  has_congestion
} 0

check "Uncongested -congestion_only run truncates report file" {
  file size $rpt_file
} 0

set bad_rpt_file [file join [make_result_file "nonexistent_congestion_dir"] \
  "congestion_only.rpt"]
check "Unwritable -congestion_report_file fails loudly with GRT-0342" {
  set rc [catch {
    global_route -congestion_only -congestion_report_file $bad_rpt_file
  } err]
  expr { $rc != 0 && [string first "GRT-0342" $err] >= 0 }
} 1
grt::set_congestion_report_file ""

set_global_routing_layer_adjustment metal1-metal10 0.9
check "A congested estimate warns instead of failing without -allow_congestion" {
  catch { global_route -congestion_only }
} 0
set_global_routing_layer_adjustment metal1-metal10 0.0

check "CUGR rejects -congestion_only with GRT-0340" {
  set rc [catch { global_route -use_cugr -congestion_only } err]
  expr { $rc != 0 && [string first "GRT-0340" $err] >= 0 }
} 1

exit_summary

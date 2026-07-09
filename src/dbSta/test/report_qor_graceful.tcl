# report_qor graceful-degradation path.
#
# A design with NO clock, NO parasitics, NO liberty (LEF+DEF only). report_qor
# must run without error and mark timing / power / clock / DRC as n/a while
# still reporting the always-available design size and area sections.
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def aocv_derate.def

# Must not error on a partially-set-up design.
set rc [catch { report_qor } err]
puts "report_qor_no_error=[expr { $rc == 0 }]"
if { $rc != 0 } {
  puts "error: $err"
}

set txt [report_qor]

# Design size is always available from odb.
set block [ord::get_db_block]
puts "instances_nonzero=[expr { [llength [$block getInsts]] > 0 }]"

# The prerequisite-gated sections must degrade to n/a, not error or fake data.
puts "timing_na=[expr { [string first {n/a (no clock} $txt] >= 0 }]"
puts "power_na=[expr { [string first {n/a (no liberty} $txt] >= 0 }]"
puts "drc_na=[expr { [string first {n/a (no markers} $txt] >= 0 }]"

# JSON path degrades to null for the missing sections.
set js [report_qor -json]
puts "json_no_error=1"
puts "json_timing_null=[expr { [string first {"timing": null} $js] >= 0 }]"
puts "json_power_null=[expr { [string first {"power": null} $js] >= 0 }]"
puts "json_drc_null=[expr { [string first {"drc_violations": null} $js] >= 0 }]"

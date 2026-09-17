# A constraint scoped to an analysis corner lives outside the record, in
# a (mode, corner) overlay Sdc. write_db -sdc refuses the whole record
# rather than store one that quietly leaves the corner out, and says
# which corner made it refuse.
source "sdc_in_db_common.tcl"

set ::env(SDC_IN_DB_ODB) [make_result_file sdc_in_db14.odb]
set child [run_child sdc_in_db_corner.tcl]
if { [string match "*no analysis corners*" $child] } {
  # Stock OpenSTA: no corner scope exists, so there is nothing to refuse.
  check "no corner scope to refuse" 1 1
  exit_summary
}
check "form stored by the refusing process" { child_kind $child } none
check "refusal names the corner" { regexp {slow} $child } 1
exit_summary

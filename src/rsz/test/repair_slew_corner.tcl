# Slew pre-checks use the load's corner, independent of corner definition order.
source "helpers.tcl"
set test_name repair_slew_corner
set stream [open ${test_name}.lib r]
set macro_template [read $stream]
close $stream
set std_libs {tt sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
              ss sky130hd/sky130_fd_sc_hd__ss_n40C_1v40.lib}

set cases {
  {tt}    0.04  1.5   {}
  {ss}    0.04  1.5   {}
  {tt ss} 0.04  1.5   {}
  {ss tt} 0.04  1.5   {}
  {tt}    0.03  1.5   {}
  {tt ss} 0.03  1.5   {}
  {ss tt} 0.03  1.5   {}
  {tt ss} 0.001 1.5   tt
  {tt ss} 0.04  0.001 ss
}

if { ![info exists ::env(RSZ_SLEW_CORNER_CASE)] } {
  for { set index 0 } { $index < [llength $cases] / 4 } { incr index } {
    set ::env(RSZ_SLEW_CORNER_CASE) $index
    set failed [catch {
      exec [info nameofexecutable] -no_splash -no_init -exit \
        -threads [thread_count] [info script] 2>@1
    } output]
    puts $output
    check "corner case $index completes" { set failed } 0
  }
  unset ::env(RSZ_SLEW_CORNER_CASE)
  exit_summary
}

set offset [expr { 4 * $::env(RSZ_SLEW_CORNER_CASE) }]
lassign [lrange $cases $offset [expr { $offset + 3 }]] \
  corners tt_limit ss_limit error_corner

read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_lef ${test_name}.lef
define_corners {*}$corners
foreach corner $corners {
  read_liberty -corner $corner [dict get $std_libs $corner]
  set limit [expr { $corner eq "tt" ? $tt_limit : $ss_limit }]
  set macro_file [make_result_file ${test_name}_${corner}.lib]
  set stream [open $macro_file w]
  puts $stream [string map [list macro_tt macro_$corner \
    {max_transition : 0.04} "max_transition : $limit"] $macro_template]
  close $stream
  read_liberty -corner $corner $macro_file
}
read_def ${test_name}.def
set_wire_rc -layer met2
estimate_parasitics -placement

set failed [catch { tee -quiet -variable log { repair_design } } message]
if { $error_corner eq "" } {
  if { $failed } { puts $log }
  check "$corners accepts the achievable limits" { set failed } 0
  if { "tt" in $corners } {
    check "$corners inserted a repair buffer" {
      expr { [llength [[ord::get_db_block] getInsts]] > 2 }
    } 1
  }
} else {
  check "$corners rejects the impossible $error_corner limit" { set failed } 1
  check "failure is RSZ-0090" { set message } RSZ-0090
  check "error names the load and corner" {
    string match "*pin u_macro/A at corner $error_corner *" $log
  } 1
  check "error does not assume the limit came from SDC" {
    string match {*from SDC*} $log
  } 0
}
exit_summary

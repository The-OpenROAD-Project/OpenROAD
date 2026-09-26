# openroad reads the ~/.openroad init file with Tcl_EvalFile, the C api
# behind the tcl source command.  An error in it is reported with the
# tcl stack trace that produced it, and openroad goes on to read
# cmd_file as before.  HOME is pointed at a result directory holding an
# .openroad that sets a variable, then fails two proc frames down.
source "helpers.tcl"

set home [make_result_test_dir init_file_error_home]
set init [file join $home .openroad]
set stream [open $init w]
puts $stream {
set init_file_error_was_read 1
proc init_file_error_inner { x } {
  return [expr { $x / 0 }]
}
proc init_file_error_outer { } {
  init_file_error_inner 3
}
init_file_error_outer
}
close $stream

set openroad [info nameofexecutable]
set status 0
if {
  [catch {
    exec env HOME=$home $openroad -no_splash -exit \
      init_file_error_cmd.tcl 2>@1
  } out]
} {
  set status [lindex $::errorCode 2]
}

set failures 0

proc check { description ok } {
  global failures out
  if { !$ok } {
    incr failures
    puts "FAIL: $description, openroad said:"
    puts $out
  }
}

check "the init file is read" \
  [string match {*init file was read*} $out]
check "the error message is reported" \
  [string match {*divide by zero*} $out]
check "the failing proc is named" \
  [string match {*procedure "init_file_error_inner"*} $out]
check "its caller is named" \
  [string match {*procedure "init_file_error_outer"*} $out]
# Tcl elides a long path in a stack trace, so the file is matched on its
# line alone.
check "the failing line of the init file is named" \
  [string match {*(file "*" line 9)*} $out]
check "cmd_file still runs after an init file error" \
  [string match {*cmd_file ran*} $out]
check "openroad exits zero, status was $status" \
  [expr { $status == 0 }]

if { $failures == 0 } {
  puts pass
}

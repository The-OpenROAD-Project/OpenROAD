# openroad reads cmd_file with Tcl_EvalFile, the C api behind the tcl
# source command, so an error in it is reported with the tcl stack trace
# that produced it.  cmd_file_error_inner.tcl fails two proc frames down,
# so the test is whether those frames are named.

set openroad [info nameofexecutable]
# Relative: the regression harness runs the test with the test directory
# as the working directory, and tcl elides a long path in a stack trace.
set failing cmd_file_error_inner.tcl

# The script under test fails, so openroad exits non-zero; catch keeps
# the output and the exit status either way.
set status 0
if { [catch { exec $openroad -no_init -no_splash -exit $failing 2>@1 } out] } {
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

check "the error message is reported" \
  [string match {*divide by zero*} $out]
check "the failing proc is named" \
  [string match {*procedure "cmd_file_error_inner"*} $out]
check "its caller is named" \
  [string match {*procedure "cmd_file_error_outer"*} $out]
check "the command file is named" \
  [string match {*file "cmd_file_error_inner.tcl" line 12*} $out]
check "openroad exits non-zero, status was $status" \
  [expr { $status != 0 }]

if { $failures == 0 } {
  puts pass
}

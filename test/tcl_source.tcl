# -tcl_source reads the command file with tcl's own source command
# instead of sta::include_file.  include_file reads the file one command
# at a time, which is what supports -echo, -verbose,
# sta_continue_on_error and gzipped files, but an error read that way is
# reported without the tcl stack trace that produced it.
#
# tcl_source_error.tcl fails two proc frames down, so the difference is
# whether those frames are named.

set openroad [info nameofexecutable]
# Relative: the regression harness runs the test with the test directory
# as the working directory, and tcl elides a long path in a stack trace.
set failing tcl_source_error.tcl

proc run_openroad { args } {
  # The script under test fails, so openroad exits non-zero; catch keeps
  # the output either way.
  catch { exec {*}$args 2>@1 } out
  return $out
}

set with [run_openroad $openroad -no_init -no_splash -exit -tcl_source $failing]
set without [run_openroad $openroad -no_init -no_splash -exit $failing]

set failures 0

proc check { description ok output } {
  global failures
  if { !$ok } {
    incr failures
    puts "FAIL: $description, openroad said:"
    puts $output
  }
}

check "-tcl_source names the failing proc" \
  [string match {*procedure "tcl_source_inner"*} $with] $with
check "-tcl_source names its caller" \
  [string match {*procedure "tcl_source_outer"*} $with] $with
check "-tcl_source names the command file" \
  [string match {*tcl_source_error.tcl*} $with] $with
check "the default reader still reports the error" \
  [string match {*divide by zero*} $without] $without
check "the default reader is left alone, and reports no stack trace" \
  [expr { ![string match {*procedure*} $without] }] $without

if { $failures == 0 } {
  puts pass
}

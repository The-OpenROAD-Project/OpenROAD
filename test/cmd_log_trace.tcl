# Test command tracing into the OpenROAD log file.
# Expected invocation (from regression):
#   openroad -no_init -no_splash -exit -log <result_log> cmd_log_trace.tcl
#
# The regression harness redirects stdout/stderr to the same <result_log> file.
# This script verifies that command tracing appended command lines to that file.

source "helpers.tcl"

puts "TEST: Verifying command tracing in log"

# Find the log path from argv: ... -log <file> ...
set log_file ""
set argc [llength $argv]
for {set i 0} {$i < $argc} {incr i} {
  if {[lindex $argv $i] == "-log"} {
    if {$i + 1 < $argc} {
      set log_file [lindex $argv [expr {$i + 1}]]
    }
    break
  }
}

if {$log_file == ""} {
  puts "FAIL: Could not find -log file argument"
  exit 1
}

if {![file exists $log_file]} {
  puts "FAIL: Log file does not exist: $log_file"
  exit 1
}

set fp [open $log_file r]
set log_text [read $fp]
close $fp

# Look for traced commands emitted by Main.cc callback:
#   cmd: <command>
set expected_cmds {
  {cmd: source {helpers.tcl}}
  {cmd: puts {TEST: Verifying command tracing in log}}
}

set missing {}
foreach pat $expected_cmds {
  if {[string first $pat $log_text] == -1} {
    lappend missing $pat
  }
}

if {[llength $missing] > 0} {
  puts "FAIL: Missing traced command lines:"
  foreach m $missing {
    puts "  $m"
  }
  exit 1
}

puts "PASS: Found traced command lines in log"
puts "PASS: Command log trace test completed successfully"

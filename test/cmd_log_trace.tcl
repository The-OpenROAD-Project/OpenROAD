source "helpers.tcl"
puts "TEST: Verifying command tracing in log"

set result_dir [make_result_dir]
set log_file [file join $result_dir "cmd_log_trace_output.log"]
set test_script [file join $result_dir "test_inner.tcl"]
set nested_file [file join $result_dir "nested.tcl"]

# 3. NESTED SOURCE CASE
set f [open $nested_file w]
puts $f "suppress_message ORD 30"
close $f

set f [open $test_script w]
puts $f "suppress_message ODB 127"
puts $f "help help"
puts $f "::help help"
puts $f "set x 1"
puts $f "if { \$x == 1 } { puts \"internal_skip_if\" }"
puts $f "while { \$x < 1 } { puts \"internal_skip_while\" }"
puts $f "source \"$nested_file\""
close $f

set ord_exe [info nameofexecutable]

# After the exec/run step
catch { exec $ord_exe -log $log_file -no_splash -no_init -exit $test_script }

# 4. explicitly check file exists before opening it
if { ![file exists $log_file] } {
  puts "FAIL: Log file does not exist: $log_file"
  exit 1
}

set f [open $log_file r]
set content [read $f]
close $f

# 1. A real OR command appears as "cmd: <command>"
if {
  [regexp "cmd: suppress_message ODB 127" $content] &&
  [regexp "cmd: help help" $content] &&
  [regexp "cmd: ::help help" $content]
} {
  puts "PASS: Found logged commands"
} else {
  puts "FAIL: Logged commands not found"
  exit 1
}

if { [regexp "cmd: suppress_message ORD 30" $content] } {
  puts "PASS: Found nested sourced command"
} else {
  puts "FAIL: Nested sourced command not found"
  exit 1
}

# 2. Tcl builtins do NOT appear:
# assert "cmd: set", "cmd: if", "cmd: puts", "cmd: while" are all absent
if {
  [regexp "cmd: set" $content] || [regexp "cmd: if" $content] ||
  [regexp "cmd: puts" $content] || [regexp "cmd: while" $content]
} {
  puts "FAIL: internal commands logged"
  exit 1
} else {
  puts "PASS: internal commands not logged"
}

puts "pass"

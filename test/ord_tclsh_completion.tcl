# Mock tclreadline
namespace eval ::tclreadline {
proc Lindex { line index } {
  return [lindex $line $index]
}

proc ScriptCompleter { part start end line } {
  # Simulate noisy output from ScriptCompleter to ensure it's suppressed
  puts "ScriptCompleter: [ERROR] Should be suppressed"
  error "ScriptCompleter Error"
}

proc RemoveUsedOptions { line flags } {
  # Default behavior: return flags as is
  return $flags
}

proc CompleteFromList { part matches } {
  set res {}
  foreach m $matches {
    if { [string match "${part}*" $m] } {
      lappend res $m
    }
  }
  return $res
}
}

if { [info procs ::ord::cmd_args_completer] eq "" } {
  puts "Error: ::ord::cmd_args_completer not found."
  exit 1
}

puts "Test 1: Command name completion 'read_db'"
# start=0 triggers ScriptCompleter path.
# We expect NO error to propagate (caught inside) and clean stdout (redirected).
# Result should be "" (from catch block returning "")
set result ""
if { [catch { set result [::ord::cmd_args_completer "read_db" 0 7 "read_db"] } err] } {
  puts "FAIL: Test 1 threw error: $err"
} else {
  if { $result eq "" } {
    puts "PASS: Test 1 returned empty (as expected for mock failure)"
  } else {
    puts "FAIL: Test 1 returned: $result"
  }
}

puts "\nTest 2: Argument completion 'read_db '"
# start=8 triggers RemoveUsedOptions path.
# We expect it to find files in current directory (or our mock).
# Since we didn't mock glob, it will use real glob.
# 'read_db' takes a file. cmd_args_completer logic does `glob`.
set result ""
if { [catch { set result [::ord::cmd_args_completer "" 8 8 "read_db "] } err] } {
  puts "FAIL: Test 2 threw error: $err"
} else {
  # It should return a list of files/dirs
  if { [llength $result] > 0 } {
    puts "PASS: Test 2 returned list of length > 0"
  } else {
    puts "FAIL: Test 2 returned list of length 0"
  }
}

puts "\nTest 3: write_lef completion"
# write_lef also takes filename.
if { [catch { set result [::ord::cmd_args_completer "" 10 10 "write_lef "] } err] } {
  puts "FAIL: Test 3 threw error: $err"
} else {
  if { [llength $result] > 0 } {
    puts "PASS: Test 3 returned list of length > 0"
  } else {
    puts "FAIL: Test 3 returned list of length 0"
  }
}

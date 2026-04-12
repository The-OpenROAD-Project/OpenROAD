source "helpers.tcl"

# Test that read_3dbx gives a proper error for a non-existent file
# rather than segfaulting (regression test for issue #10082)

# Test 1: Main file does not exist
if { [catch {read_3dbx "nonexistent_file.3dbx"} err_msg] } {
  if { [string first "does not exist" $err_msg] < 0 } {
    puts "FAIL: Wrong error for missing main file: $err_msg"
    exit 1
  }
  puts "Caught expected error: main file not found"
} else {
  puts "FAIL: Expected an error for non-existent main file"
  exit 1
}

# Test 2: Main file exists but references a non-existent include
if { [catch {read_3dbx "data/fail.3dbx"} err_msg] } {
  if { [string first "does not exist" $err_msg] < 0 } {
    puts "FAIL: Wrong error for missing include file: $err_msg"
    exit 1
  }
  puts "Caught expected error: include file not found"
} else {
  puts "FAIL: Expected an error for non-existent include file"
  exit 1
}

puts "pass"

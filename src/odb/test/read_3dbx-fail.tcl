source "helpers.tcl"

# Test that read_3dbx gives a proper error for a non-existent file
# rather than segfaulting (regression test for issue #10082)

# Test 1: Main file does not exist (Tcl proc raises ORD-0072 before C++ layer)
if { [catch {read_3dbx "nonexistent_file.3dbx"} err_msg] } {
  if { [string first "ORD-0072" $err_msg] < 0 } {
    puts "FAIL: Wrong error for missing main file: $err_msg"
    exit 1
  }
  puts "Caught expected error: main file not found"
} else {
  puts "FAIL: Expected an error for non-existent main file"
  exit 1
}

# Test 2: Main file exists but references a non-existent include (ODB-0521 from BaseParser::logError)
if { [catch {read_3dbx "data/fail.3dbx"} err_msg] } {
  if { [string first "ODB-0521" $err_msg] < 0 } {
    puts "FAIL: Wrong error for missing include file: $err_msg"
    exit 1
  }
  puts "Caught expected error: include file not found"
} else {
  puts "FAIL: Expected an error for non-existent include file"
  exit 1
}

# Test 3: 3DBV chiplet references a non-existent DEF file (ODB-0557 from createChiplet)
if { [catch {read_3dbv "data/fail_def.3dbv"} err_msg] } {
  if { [string first "ODB-0557" $err_msg] < 0 } {
    puts "FAIL: Wrong error for missing DEF file: $err_msg"
    exit 1
  }
  puts "Caught expected error: DEF file not found"
} else {
  puts "FAIL: Expected an error for non-existent DEF file"
  exit 1
}

puts "pass"

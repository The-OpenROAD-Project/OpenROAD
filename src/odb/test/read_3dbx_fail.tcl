# Regression for issue #10082: read_3dbx must report a clean error (not crash)
# on an invalid / nonexistent path, instead of proceeding to create DB objects
# with an uninitialized state and segfaulting.
source "helpers.tcl"

# Test 1: nonexistent file passed to read_3dbx.
# Caught by the read_3dbx Tcl wrapper before reaching the C++ reader.
set status [catch { read_3dbx "data/this_file_does_not_exist.3dbx" } msg]
if { $status != 1 } {
  puts "fail: read_3dbx accepted a nonexistent path"
  exit 1
}
if { ![regexp "ORD-0072" $msg] } {
  puts "fail: unexpected error for nonexistent path: $msg"
  exit 1
}

# Test 2: invalid path that exists but is not a regular file (a directory).
# This bypasses the Tcl wrapper's "file exists" check and exercises the C++
# 3dblox reader's file-validation guard directly. Before the fix the reader's
# std::ifstream::is_open() succeeded on a directory (on Linux), so parsing
# proceeded on empty content and could create DB objects with uninitialized
# state -> segfault. Call the C++ command directly to reach that path.
set dir_path [make_result_file "read_3dbx_fail_dir.3dbx"]
file delete -force $dir_path
file mkdir $dir_path
set status [catch { ord::read_3dbx_cmd $dir_path } msg]
file delete -force $dir_path
if { $status != 1 } {
  puts "fail: read_3dbx_cmd accepted a directory path"
  exit 1
}
if { ![regexp "UTL-0015" $msg] } {
  puts "fail: unexpected error for directory path: $msg"
  exit 1
}

puts "pass"

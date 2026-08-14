source "helpers.tcl"

# Test 1: Region with no coordinates
set status [catch { read_3dbv "data/example_no_coords.3dbv" } msg]
if { $status != 1 } {
  puts "fail: read_3dbv accepted region without coordinates"
  exit 1
}
if { ![regexp "ODB-0521" $msg] } {
  puts "fail: unexpected error for no coords: $msg"
  exit 1
}

# Test 2: Region with polygon (5 coordinates)
set status [catch { read_3dbv "data/example_polygon.3dbv" } msg]
if { $status != 1 } {
  puts "fail: read_3dbv accepted region with 5 coordinates (polygon)"
  exit 1
}
if { ![regexp "ODB-0521" $msg] } {
  puts "fail: unexpected error for polygon: $msg"
  exit 1
}

puts "pass"

source "helpers.tcl"

read_lef "data/gscl45nm.lef"

set status [catch { read_def "data/virtual_route_invalid.def" } msg]
if { $status != 1 } {
  puts "fail: read_def accepted VIRTUAL without a prior routing point"
  exit 1
}

if { $msg != "ODB-0421" } {
  puts "fail: unexpected error: $msg"
  exit 1
}

puts "pass"

source "helpers.tcl"

read_verilog data/mpd_top/mpd_top.v
link_design mpd_top

# Valid versions should be accepted and stored.
upf_version 2.1

set db [ord::get_db]
set block [$db getChip getBlock]
set stored [$block getUPFVersion]
if { $stored ne "2.1" } {
  utl::error UPF 900 "Expected version '2.1', got '$stored'"
}

# Last write still wins for supported versions.
upf_version 3.0
set stored2 [$block getUPFVersion]
if { $stored2 ne "3.0" } {
  utl::error UPF 901 "Expected version '3.0' after update, got '$stored2'"
}

set upf_file [make_result_file upf_version.upf]
write_upf $upf_file
set fh [open $upf_file r]
set upf_text [read $fh]
close $fh
if { [string first "upf_version 3.0" $upf_text] < 0 } {
  utl::error UPF 903 "write_upf did not emit the selected UPF version"
}

foreach invalid_version {abc {}} {
  set cmd [list upf_version $invalid_version]
  if { [catch $cmd msg] } {
    puts "Expected failure caught for '$invalid_version': $msg"
  } else {
    utl::error UPF 902 "upf_version '$invalid_version' should have failed"
  }
}

puts "upf_version tests passed"

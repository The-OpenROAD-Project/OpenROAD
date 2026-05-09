source "helpers.tcl"

if { ![gui::supported] } {
  puts "Pass"
  exit
}

suppress_message ODB 128
suppress_message ODB 130
suppress_message ODB 131
suppress_message ODB 132
suppress_message ODB 133
suppress_message ODB 227

read_lef Nangate45/Nangate45.lef
read_def gcd_nangate45.def

set heatmap_file [make_result_file dump_heatmap_headless.csv]
gui::dump_heatmap Placement $heatmap_file

set heatmap [open $heatmap_file r]
set contents [read $heatmap]
close $heatmap

if { [string first "value (%)" $contents] >= 0 \
     && [regexp -line {^[0-9.-]+,[0-9.-]+,[0-9.-]+,[0-9.-]+,} $contents] } {
  puts "Pass"
} else {
  puts "Fail"
}

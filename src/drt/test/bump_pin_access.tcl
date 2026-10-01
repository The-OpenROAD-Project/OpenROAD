# A bump (CLASS COVER BUMP) is an IO terminal reached from below: the route
# must drop a via into the bump pad rather than enter it on the pad layer.
source "helpers.tcl"

read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef bump_pin_access.lef
read_def bump_pin_access.def

make_tracks
set_routing_layers -signal metal2-metal10
global_route
set drc_file [make_result_file bump_pin_access.drc]
detailed_route -verbose 0 -output_drc $drc_file

set violations 0
set fh [open $drc_file]
foreach line [split [read $fh] "\n"] {
  if { [string match "violation type:*" $line] } {
    incr violations
  }
}
close $fh
puts "violations: $violations"

set def_file [make_result_file bump_pin_access.def]
write_def $def_file
diff_files bump_pin_access.defok $def_file

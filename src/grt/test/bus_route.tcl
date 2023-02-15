source "helpers.tcl"
read_liberty "sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib"
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def bus_route.def

set guide_file [make_result_file bus_route.guide]

set_routing_layers -signal met1-met5
global_route

write_guides $guide_file

set stream [open $guide_file r]
set lines [split [read $stream] "\n"]
close $stream

foreach line $lines {
  if {[regexp {met[3-5]} $line] != 0} {
    puts "FAIL: met3-5 found"
    exit
  }
}

puts "SUCCESS"

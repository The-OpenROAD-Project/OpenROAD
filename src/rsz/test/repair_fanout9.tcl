# repair_design default fanout backstop without liberty fanout-load data
source "helpers.tcl"
source "hi_fanout.tcl"

set test_name repair_fanout9
set source_lib repair_fanout6.lib
set lib_file [make_result_file "${test_name}.lib"]
set stream [open $source_lib r]
set liberty [read $stream]
close $stream
# The test library has no per-port fanout loads, so remove its default too.
regsub -all {default_fanout_load[ \t]*:[^;]+;} $liberty "" liberty
set stream [open $lib_file w]
puts -nonewline $stream $liberty
close $stream

set def_file [make_result_file "${test_name}.def"]
write_hi_fanout_def1 $def_file 60 \
  "source" "sky130_fd_sc_hd__dfxtp_1" "CLK" "Q" \
  "sink" "sky130_fd_sc_hd__dfxtp_1" "CLK" "D" 5000 \
  "met3" 1000

read_liberty $lib_file
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def $def_file
create_clock -period 0.1 clk1

source sky130hd/sky130hd.vars
source sky130hd/sky130hd.rc
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use
estimate_parasitics -placement

repair_design

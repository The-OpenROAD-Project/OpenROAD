# estimate_parasitics -placement estimates each net on the STA's threads.
# What is intended: the thread count changes how fast, never what. This
# places aes_cipher_top and estimates its placement parasitics at 1
# thread and again at 8, times both at 1 thread, and fails unless every
# pin's slew and slack are equal.
source "helpers.tcl"
source "flow_helpers.tcl"
source "Nangate45/Nangate45.vars"

read_liberty $liberty_file
read_lef $tech_lef
read_lef $std_cell_lef
read_verilog aes_nangate45.v
link_design aes_cipher_top
read_sdc aes_nangate45.sdc

initialize_floorplan -die_area {0 0 1020 920.8} -core_area {10 12 1010 911.2} \
  -site $site
source $tracks_file
place_pins -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer
global_placement -skip_nesterov_place -density $global_place_density

source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk

proc estimate_at { threads } {
  set_thread_count $threads
  estimate_parasitics -placement
  set_thread_count 1
  sta::find_timing_cmd 1
  set timing {}
  foreach pin [get_pins -hierarchical *] {
    lappend timing [get_full_name $pin] \
      [get_property $pin slew_max] [get_property $pin slack_max]
  }
  return $timing
}

set timing_1 [estimate_at 1]
set timing_8 [estimate_at 8]

if { [llength $timing_1] == 0 } {
  utl::error EST 2100 "No pins to compare."
}
foreach {pin slew_1 slack_1} $timing_1 {pin_8 slew_8 slack_8} $timing_8 {
  if { $pin ne $pin_8 || $slew_1 ne $slew_8 || $slack_1 ne $slack_8 } {
    utl::error EST 2101 "Pin $pin with parasitics estimated at 8 threads\
      (slew $slew_8, slack $slack_8) differs from 1 thread\
      (slew $slew_1, slack $slack_1)."
  }
}

puts "pass"
exit 0

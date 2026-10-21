# Resistance-aware layer assignment estimates the global-route parasitics
# of every net, in parallel when OpenROAD has more than one thread. What is
# intended: the thread count changes how fast, never what. This routes
# gcd_asap7 resistance-aware at 1 thread and again at 4, and fails unless
# the guides are byte-identical and every pin's slew and slack, which
# follow from every net's parasitics, are equal.
source "helpers.tcl"

read_liberty asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_AO_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_LVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_LVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_LVT_FF_nldm_220123.lib
read_liberty asap7/asap7sc7p5t_AO_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_INVBUF_SLVT_FF_nldm_220122.lib.gz
read_liberty asap7/asap7sc7p5t_OA_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SIMPLE_SLVT_FF_nldm_211120.lib.gz
read_liberty asap7/asap7sc7p5t_SEQ_SLVT_FF_nldm_220123.lib
read_lef asap7/asap7_tech_1x_201209.lef
read_lef asap7/asap7sc7p5t_28_R_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_L_1x_220121a.lef
read_lef asap7/asap7sc7p5t_28_SL_1x_220121a.lef
read_def gcd_asap7_placed.def.gz
read_sdc gcd_asap7.sdc

source asap7/setRC.tcl
set_wire_rc -signal -layer M3
set_wire_rc -clock -layer M6
set_propagated_clock [all_clocks]
set_routing_layers -signal M2-M6 -clock M4-M6
set_global_routing_layer_adjustment M2-M7 0.6

proc route_at { threads } {
  set_thread_count $threads
  estimate_parasitics -placement
  set output ""
  tee -variable output -quiet {
    global_route -critical_nets_percentage 30 -resistance_aware -verbose
  }
  if { ![regexp {Total wirelength:\s+[0-9]+\s+um} $output] } {
    utl::error GRT 713 "global_route at $threads threads reported no wirelength."
  }
  set guides [make_result_file "resistance_aware_threads_$threads.guide"]
  write_guides $guides
  estimate_parasitics -global_routing
  set f [open $guides r]
  set text [read $f]
  close $f
  set timing {}
  foreach pin [get_pins -hierarchical *] {
    lappend timing [get_full_name $pin] \
      [get_property $pin slew_max] [get_property $pin slack_max]
  }
  return [list $text $timing]
}

lassign [route_at 1] guides_1 timing_1
lassign [route_at 4] guides_4 timing_4

if { $guides_1 ne $guides_4 } {
  utl::error GRT 714 "Guides at 4 threads differ from 1 thread."
}
if { [llength $timing_1] == 0 } {
  utl::error GRT 715 "No pins to compare."
}
foreach {pin slew_1 slack_1} $timing_1 {pin_4 slew_4 slack_4} $timing_4 {
  if { $pin ne $pin_4 || $slew_1 ne $slew_4 || $slack_1 ne $slack_4 } {
    utl::error GRT 716 "Pin $pin at 4 threads (slew $slew_4, slack $slack_4)\
      differs from 1 thread (slew $slew_1, slack $slack_1)."
  }
}

puts "pass"
exit 0

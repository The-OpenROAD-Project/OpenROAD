source "helpers.tcl"
source "flow_helpers.tcl"
source "sky130hd/sky130hd.vars"

proc write_regions_section { input_def output_regions } {
  set in_stream [open $input_def r]
  set out_stream [open $output_regions w]
  set copy_lines 0

  while { [gets $in_stream line] >= 0 } {
    if { [string match "REGIONS *" $line] } {
      set copy_lines 1
    }
    if { $copy_lines } {
      puts $out_stream $line
      if { [string trim $line] == "END REGIONS" } {
        break
      }
    }
  }

  close $in_stream
  close $out_stream
}

read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_aes.v
link_design mpd_top
read_upf -file upf/mpd_aes.upf
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

set_domain_area PD_AES_1 -area {30   30 650 490}
set_domain_area PD_AES_2 -area {30 510 650 970}


initialize_floorplan \
  -die_area {0 0 1000 1000} \
  -core_area {30 30 970 970} \
  -site unithd \
  -additional_site unithddbl

make_tracks

set_routing_layers -signal li1-met5

place_pins \
  -hor_layers met3 \
  -ver_layers met2
global_placement -skip_initial_place -density uniform -routability_driven -timing_driven

detailed_placement -max_displacement 650
improve_placement
check_placement

set def_file [make_result_file upf_aes.def]
write_def $def_file
if { [info exists ::env(BAZEL_TEST)] } {
  set regions_file [make_result_file upf_aes.regions]
  write_regions_section $def_file $regions_file
  diff_file $regions_file upf_aes.regionsok
} else {
  diff_file $def_file upf_aes.defok
}

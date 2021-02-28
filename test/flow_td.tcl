# assumes flow_helpers.tcl has been read
read_libraries
read_verilog $synth_verilog
link_design $top_module
read_sdc $sdc_file

initialize_floorplan -site $site \
  -die_area $die_area \
  -core_area $core_area \
  -tracks $tracks_file

# remove buffers inserted by synthesis 
remove_buffers

################################################################
# IO Placement
place_pins -random -hor_layers $io_placer_hor_layer -ver_layers $io_placer_ver_layer

################################################################
# Macro Placement
if { [have_macros] } {
  # tdms_place (but replace isn't timing driven)
  global_placement -disable_routability_driven -density $global_place_density

  macro_placement -global_config $ip_global_cfg
}

################################################################
# Tapcell insertion
eval tapcell $tapcell_args

################################################################
# Power distribution network insertion
pdngen -verbose $pdn_cfg

################################################################

set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock  -layer $wire_rc_layer_clk
set_dont_use $dont_use

# Global placement
global_placement -disable_routability_driven \
  -density $global_place_density \
  -disable_timing_driven \
  -init_density_penalty $global_place_density_penalty \
  -pad_left $global_place_pad -pad_right $global_place_pad


#puts "replace wns = [get_replace_virtual_resizer_wns]"

################################################################
# Resize
# estimate wire rc parasitics
estimate_parasitics -placement
repair_design
report_worst_slack

# Test hier module swap after placement
set ECHO_COMMANDS {
    read_lef read_def read_verilog read_sdc link_design
    initialize_floorplan place_pins
    global_placement detailed_placement
    clock_tree_synthesis route_design
    write_def write_verilog report_checks
}

# Save and wrap each command
foreach cmd $ECHO_COMMANDS {
  if { [info commands $cmd] != "" } {
    set orig_cmd "orig_$cmd"
    if { [info commands $orig_cmd] == "" } {
      rename $cmd $orig_cmd
      # tclint-disable-next-line command-args
      proc $cmd {args} "
                puts \">> $cmd \$args\"
                ${orig_cmd} {*}\$args
            "
    }
  }
}

source "helpers.tcl"
source Nangate45/Nangate45.vars
read_liberty gf180/gf180mcu_fd_sc_mcu9t5v0__tt_025C_5v00.lib.gz
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_tech.lef
read_lef gf180/gf180mcu_5LM_1TM_9K_9t_sc.lef

read_verilog jpeg.v
link_design jpeg_encoder -hier
read_sdc jpeg.sdc

set_propagated_clock [get_clocks clk]
set env(METAL_OPTION) 5
set env(CORNER) WC
source gf180/setRC.tcl

# Place the design
initialize_floorplan \
  -site GF018hv5v_green_sc9 \
  -die_area "0 0 3000 3000" \
  -core_area "10 10 2990 2990"
make_tracks
place_pins -hor_layers Metal3 -ver_layers Metal2
global_placement -skip_nesterov_place

set num_instances [llength [get_cells -hier *]]
puts "number instances in verilog is $num_instances"

set_debug_level ODB replace_design_check_sanity 1
report_wns
report_tns

estimate_parasitics -placement
replace_arith_modules -path_count 100

report_wns
report_tns

# QoR must be the same after sta::network_changed
estimate_parasitics -placement
report_wns
report_tns

set write_sdc_file [make_result_file replace_arith_modules3.sdc]
write_sdc $write_sdc_file

# Test hier module swap and reverse swap
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

#place the design
#initialize_floorplan -die_area "0 0 40 1200"   -core_area "0 0 40 1200" \
#   -site FreePDK45_38x28_10R_NP_162NW_34O
#global_placement -skip_nesterov_place
#detailed_placement

#source gf180/setRC.tcl

set num_instances [llength [get_cells -hier *]]
puts "number instances in verilog is $num_instances"

#set_debug_level RSZ replace_arith 1
report_wns
report_tns

replace_arith_modules -path_count 100

report_wns
report_tns

# QoR must be the same after sta::network_changed
estimate_parasitics -placement
report_wns
report_tns

set write_sdc_file [make_result_file replace_arith_modules1.sdc]
write_sdc $write_sdc_file

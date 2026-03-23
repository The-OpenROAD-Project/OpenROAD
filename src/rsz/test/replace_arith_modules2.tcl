# Test hier module swap with a modified netlist of unused output pin
# and deeper hierarchical modules.
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

# 'jpeg2.v' netlist has two differences from 'jpeg.v'.
#   1. More hierarchical modules wrapping the arithmetic modules, which
#      can be used for duplicate dbNet name testing among different modules.
#   2. Intentionally made unused output pin for multiplier module.
read_verilog jpeg2.v
link_design jpeg_encoder -hier
read_sdc jpeg.sdc

set num_instances [llength [get_cells -hier *]]
puts "number instances in verilog is $num_instances"

#set_debug_level RSZ replace_arith 1
#set_debug_level ODB replace_design 10
set_debug_level ODB replace_design_check_sanity 1
report_wns
report_tns

replace_arith_modules -path_count 100

report_wns
report_tns

set write_sdc_file [make_result_file replace_arith_modules2.sdc]
write_sdc $write_sdc_file

sta::check_axioms

# Ports on a pad pin stay connected to guides reloaded after pin_access.
set test_name pad_pin_edge_odb_roundtrip
source "helpers.tcl"
read_lef "gf180/gf180mcu_6LM_1TM_9K_7t_tech.lef"
read_lef "gf180/gf180mcu_fd_sc_mcu7t5v0.lef"
read_lef "pad_pin_edge.lef"
read_def "pad_pin_edge.def"
make_tracks

# Move the buffer so the guides miss the pad pin's access-point gcell.
[[ord::get_db_block] findInst buf] setLocation 1000000 1100000

set_routing_layers -signal Metal2-Metal5
global_route
write_db [make_result_file "${test_name}.odb"]

# Reload in a separate process so the guides come from the database.
puts [exec [info nameofexecutable] -no_splash -no_init -exit \
  -threads [thread_count] \
  [file join [file dirname [info script]] "${test_name}_reload.tcl"] 2>@1]

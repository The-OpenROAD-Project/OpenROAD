# Reproducer: hier write_db/read_db must preserve sub-module net
# connectivity through MBFF trays, so STA still finds paths from top
# ports through hierarchical FFs after reload in a fresh session.
#
# History: this used to drop sub-module net connectivity on reload, and
# MBFF (cluster_flops) under a -hier design was the suspected trigger.
# Full round-trip:
#   link_design -hier -> place -> cluster_flops -> write_db
#   (fresh process) read_db -> verify connectivity + report_checks
#
# Design: mbff_hier.v   top mbff_hier -> u_mid -> l0/l1 -> ff0..ff3
# Top input din[0] crosses two hier levels to reach a leaf FF D pin;
# after clustering it reaches a tray D pin inside the leaf module.

source helpers.tcl
set test_name hier_writedb_readdb_repro

read_lef ./asap7/asap7_tech_1x_201209.lef
read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef
read_lib ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib

read_lef ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X.lef
read_lib ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib
read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef
read_lib ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib

read_verilog ./mbff_hier.v
link_design -hier mbff_hier

initialize_floorplan -die_area "0 0 10 10" \
  -core_area "1 1 9 9" \
  -site asap7sc7p5t

set i 0
foreach inst [[ord::get_db_block] getInsts] {
  set x [expr 2000 + ($i % 4) * 2000]
  set y [expr 2000 + ($i / 4) * 2000]
  $inst setLocation $x $y
  $inst setPlacementStatus PLACED
  incr i
}

create_clock -name clk -period 1000 [get_ports clk1]
set_input_delay -clock clk 0 [get_ports din[0]]

cluster_flops -tray_weight 40.0 -timing_weight 0.0 \
  -max_split_size -1 -num_paths 0

# Connectivity + timing check, run identically before and after the db
# round-trip. Hard-errors if sub-module connectivity is dropped on
# reload (the bug this guards against). After clustering, top input
# din[0] must still reach a D pin of a tray placed inside leaf l0, and
# report_checks must produce a timing path through it.
proc check_din0 {phase} {
  set pins {}
  foreach p [get_pins -of_objects [get_nets din[0]]] {
    lappend pins [get_property $p full_name]
  }
  puts "--- $phase: net din\[0\] pins ---"
  foreach p $pins { puts "  $p" }
  if {[lsearch -glob $pins "u_mid/l0/*/D*"] < 0} {
    utl::error GPL 331 "$phase: net din\[0\] lost sub-module connectivity\
 (expected a u_mid/l0/.../D pin, got: $pins)."
  }
  puts "--- $phase: report_checks ---"
  report_checks -path_delay max -from [get_ports din[0]]
}

puts "=== Phase 1: live link_design -hier + cluster_flops session ==="
check_din0 phase1-live

# Persist + spawn a fresh openroad that only does read_db + constraints
# + the same connectivity/timing checks.
set db_file [make_result_file $test_name.odb]
write_db $db_file

set reload_tcl [make_result_file ${test_name}_reload.tcl]
set rf [open $reload_tcl w]
puts $rf "source helpers.tcl"
puts $rf "read_lef ./asap7/asap7_tech_1x_201209.lef"
puts $rf "read_lef ./SingleBit/asap7sc7p5t_28_L_1x_220121a.lef"
puts $rf "read_lib ./SingleBit/asap7sc7p5t_SEQ_LVT_TT_nldm_220123.lib"
puts $rf "read_lef ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X.lef"
puts $rf "read_lib ./2BitTrayH2/asap7sc7p5t_DFFHQNV2X_LVT_TT_nldm_FAKE.lib"
puts $rf "read_lef ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X.lef"
puts $rf "read_lib ./4BitTrayH4/asap7sc7p5t_DFFHQNV4X_LVT_TT_nldm_FAKE.lib"
puts $rf "read_db $db_file"
puts $rf "create_clock -name clk -period 1000 \[get_ports clk1\]"
puts $rf "set_input_delay -clock clk 0 \[get_ports din\[0\]\]"
puts $rf [list proc check_din0 {phase} [info body check_din0]]
puts $rf "check_din0 phase2-fresh-reload"
close $rf

puts ""
puts "=== Phase 2: fresh openroad reads db, no link_design -hier ==="
catch {exec [info nameofexecutable] -no_init -exit $reload_tcl 2>@1} reload_out
puts $reload_out

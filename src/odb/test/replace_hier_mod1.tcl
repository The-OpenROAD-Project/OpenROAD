# Test hier module swap and reverse swap

source "helpers.tcl"
source Nangate45/Nangate45.vars
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef

read_verilog replace_hier_mod1.v
link_design top -hier
create_clock -period 0.3 clk

#place the design
initialize_floorplan -die_area "0 0 40 1200" -core_area "0 0 40 1200" \
  -site FreePDK45_38x28_10R_NP_162NW_34O
source $tracks_file
place_pins -hor_layers $io_placer_hor_layer \
  -ver_layers $io_placer_ver_layer
global_placement -skip_nesterov_place
detailed_placement

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3

# For eqy, write a verilog before replace_hier_module
write_verilog_for_eqy replace_hier_mod1 before "None"

puts "### Initial bc1 is buffer_chain ###"
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through r2/D -digits 3
# Using "-through u1z" causes crash.
# - it looks like "-through" creates a cache internally.
#report_checks -through u1z -through r2/D -digits 3
puts "Equivalence check - pre"
run_equivalence_test replace_hier_mod1 \
  -lib_dir ./Nangate45/work_around_yosys/ \
  -remove_cells "None"

puts "### swap bc1 to inv_chain ###"
#set_debug_level ODB replace_design 1
set_debug_level ODB replace_design_check_sanity 1
replace_hier_module bc1 inv_chain
global_placement -skip_nesterov_place -incremental
detailed_placement
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through r2/D -digits 3
#report_checks -through u1z -through r2/D -digits 3
puts "Equivalence check - swap (buffer_chain -> inv_chain)"
run_equivalence_test replace_hier_mod1 \
  -lib_dir ./Nangate45/work_around_yosys/ \
  -remove_cells "None"

puts "### swap bc1 back to buffer_chain ###"
replace_hier_module bc1 buffer_chain
global_placement -skip_nesterov_place -incremental
detailed_placement
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through r2/D -digits 3
#report_checks -through u1z -through r2/D -digits 3
puts "Equivalence check - swap for rollback (inv_chain -> buffer_chain)"
run_equivalence_test replace_hier_mod1 \
  -lib_dir ./Nangate45/work_around_yosys/ \
  -remove_cells "None"

puts "### swap bc1 back to inv_chain ###"
replace_hier_module bc1 inv_chain
global_placement -skip_nesterov_place -incremental
detailed_placement
report_cell_usage bc1
report_net u1z -digits 3
report_net u3z -digits 3
estimate_parasitics -placement
report_checks -through r2/D -digits 3
#report_checks -through u1z -through r2/D -digits 3
puts "Equivalence check - redo swap (buffer_chain -> inv_chain)"
run_equivalence_test replace_hier_mod1 \
  -lib_dir ./Nangate45/work_around_yosys/ \
  -remove_cells "None"

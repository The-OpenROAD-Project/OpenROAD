# Gate cloning in a hierarchical design where the cloned driver and the
# moved loads live in different modules. b0 sits at the top and drives
# loads inside u1, so moving loads to the top-level clone must go through
# dbNetwork::hierarchicalConnect() and punch a new port into submodule.
source "helpers.tcl"
source Nangate45/Nangate45.vars
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_verilog clone_hier_cross.v
link_design reg1 -hier

# Load the placed fixture to keep this test independent of floorplan and placement.
read_def -floorplan_initialize clone_hier_cross.def

create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack -max

# Run only gate cloning so that b0/Z is cloned at the top
repair_timing -setup -sequence clone -skip_last_gasp

report_worst_slack -max

# Print the flat net and the module net of each load of b0 inside u1.
# Loads moved to clone1 sit on the new submodule port net_i.
set block [ord::get_db_block]
foreach load {u1/A r3/D r4/D r5/D r6/D r7/D r8/D r9/D r10/D r11/D r12/D} {
  lassign [split $load "/"] inst_name term_name
  set iterm [[$block findInst "u1/$inst_name"] findITerm $term_name]
  set mod_net [$iterm getModNet]
  set mod_name [expr { $mod_net eq "NULL" ? "none" : [$mod_net getName] }]
  puts "u1/$load flat: [[$iterm getNet] getName] mod: $mod_name"
}

set verilog_file [make_result_file clone_hier_cross_out.v]
write_verilog $verilog_file
diff_files $verilog_file clone_hier_cross_out.vok

sta::check_axioms

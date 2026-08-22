# ECO capture/replay round-trip.
#
# Exercises the user-facing begin_eco / write_eco / read_eco commands:
#   1. Snapshot a placed design.
#   2. begin_eco, run repair_timing -setup (real netlist edits), write_eco.
#   3. undo_eco back to the pristine netlist (proves the edits were real).
#   4. read_eco replays the persisted ECO file.
#   5. Assert the replayed netlist matches the modified netlist exactly,
#      proving the write->read round-trip is faithful.
#
# Note: repair_timing manages the ECO journal internally (nested begin/commit).
# An outer begin_eco accumulates those committed changes, so write_eco is issued
# while the outer journal is still open (no end_eco needed for capture).
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup1.def
create_clock -period 0.3 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Signature of the netlist: sorted "<inst>:<master>" plus the net->pin
# connectivity, so resizes (master swap) and buffer insertion/removal
# (instance + net create/destroy + connect/disconnect) are all captured.
proc netlist_signature { } {
  set block [[[ord::get_db] getChip] getBlock]
  set sig {}
  foreach inst [$block getInsts] {
    lappend sig "I [$inst getName] [[$inst getMaster] getName]"
  }
  foreach net [$block getNets] {
    set conns {}
    foreach iterm [$net getITerms] {
      lappend conns "[[$iterm getInst] getName]/[[$iterm getMTerm] getName]"
    }
    lappend sig "N [$net getName] [lsort $conns]"
  }
  return [lsort $sig]
}

proc inst_count { } {
  return [llength [[[[ord::get_db] getChip] getBlock] getInsts]]
}

set sig_orig [netlist_signature]
puts "original insts [inst_count]"

# --- Phase 1: capture the timing fix as an ECO ---
set block [[[ord::get_db] getChip] getBlock]
begin_eco
repair_timing -setup

set sig_mod [netlist_signature]
puts "modified insts [inst_count]"
puts "eco changed netlist [expr { $sig_mod ne $sig_orig ? "yes" : "no" }]"

set eco_file [make_result_file eco_round_trip.eco]
write_eco $eco_file
puts "eco file non-empty [expr { [file exists $eco_file] && [file size $eco_file] > 0 ? "yes" : "no" }]"

# --- Phase 2: undo back to the pristine netlist ---
odb::dbDatabase_undoEco $block
estimate_parasitics -placement
set sig_after_undo [netlist_signature]
puts "undo restored original [expr { $sig_after_undo eq $sig_orig ? "yes" : "no" }]"
puts "post-undo insts [inst_count]"

# --- Phase 3: replay the persisted ECO and verify equivalence ---
read_eco $eco_file
estimate_parasitics -placement
set sig_replay [netlist_signature]
puts "replayed insts [inst_count]"
puts "replay matches modified [expr { $sig_replay eq $sig_mod ? "yes" : "no" }]"

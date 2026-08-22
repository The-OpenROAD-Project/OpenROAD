# Array (high-fanout synthetic) bit-identity + perf harness.
# Reads a pre-built array checkpoint DB, runs CTS only, dumps sorted clock
# iterm positions for bit-identity comparison.
# Usage: READ_DB=<odb> openroad -exit -no_init -threads N array_cts_bench.tcl
# Env: CTS_OUT (tree dump file, default array_cts_tree.txt)
set cts_test_dir [file normalize [file dirname [info script]]]
cd $cts_test_dir
source "Nangate45/Nangate45.vars"

read_lib $liberty_file
read_lib array_tile.lib
read_lef $tech_lef
read_lef $std_cell_lef
read_lef array_tile.lef
read_db $env(READ_DB)

create_clock -period 5 clk
source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

sta::set_thread_count 1

set_cts_config -sink_clustering_max_diameter $cts_cluster_diameter \
  -root_buf $cts_buffer -buf_list $cts_buffer

set t0 [clock microseconds]
clock_tree_synthesis -sink_clustering_enable
set t1 [clock microseconds]
puts "CTS_WALL_US [expr {$t1 - $t0}]"

set out "array_cts_tree.txt"
if { [info exists ::env(CTS_OUT)] } { set out $::env(CTS_OUT) }
set block [ord::get_db_block]
set lines {}
foreach net [$block getNets] {
  if { [$net getSigType] != "CLOCK" } { continue }
  foreach iterm [$net getITerms] {
    set inst [$iterm getInst]
    set master [[$inst getMaster] getName]
    lassign [$inst getOrigin] x y
    lappend lines "[$inst getName] $master $x $y [[$iterm getMTerm] getName]"
  }
}
set fp [open $out w]
foreach l [lsort $lines] { puts $fp $l }
close $fp
puts "TREE_LINES [llength $lines]"

# aes (placed, ~530 real sinks) bit-identity + perf harness.
# Reads the pre-CTS checkpoint built by the profiling slice and runs CTS only.
# Usage: openroad -exit -no_init -threads N aes_cts_bench.tcl
# Env: CTS_OUT (tree dump file, default aes_cts_tree.txt),
#      AES_ODB  (checkpoint path; default points at profile-cts prof dir)
set test_dir [file normalize [file join [file dirname [info script]] .. test]]
cd $test_dir
source "helpers.tcl"
source "../../../test/flow_helpers.tcl"
source "Nangate45/Nangate45.vars"

set odb "/home/ubuntu/workplace/or-agents/profile-cts/prof/aes_nangate45_prects.odb"
if { [info exists ::env(AES_ODB)] } { set odb $::env(AES_ODB) }

read_libraries
read_db $odb
read_sdc "../../../test/aes_nangate45.sdc"

source $layer_rc_file
set_wire_rc -signal -layer $wire_rc_layer
set_wire_rc -clock -layer $wire_rc_layer_clk
set_dont_use $dont_use

sta::set_thread_count 1

set cts_buffer "BUF_X4"
set cts_cluster_diameter 100

set t0 [clock microseconds]
clock_tree_synthesis -root_buf $cts_buffer -buf_list $cts_buffer \
  -sink_clustering_enable \
  -sink_clustering_max_diameter $cts_cluster_diameter
set t1 [clock microseconds]
puts "CTS_WALL_US [expr {$t1 - $t0}]"

set out "aes_cts_tree.txt"
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

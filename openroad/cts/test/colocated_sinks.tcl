# Test: CTS handles co-located ICGs whose sinksBbox centers coincide.
#
# Related issue #9816
# Two ICGs (icg_a, icg_b) drive FFs arranged symmetrically around the
# same center point (100000, 100000).  cloneClockGaters() moves each ICG
# to its sinksBbox center, producing identical positions.  Without the
# fix in findLongEdges(), mapLocationToSink_ (single-valued map) silently
# overwrites the first ICG entry, losing it from the clock tree.
# The fix nudges co-located clones by 1 DBU.
source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def colocated_sinks.def

create_clock -period 5 clk

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal1
set_wire_rc -clock -layer metal2

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -buf_list CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -num_static_layers 1

# Verify both ICGs are connected to CTS buffer drivers after CTS.
# Without the fix, one ICG is lost from the tree due to map collision.
set block [ord::get_db_block]
set missing 0

foreach icg_name {icg_a icg_b} {
  set inst [$block findInst $icg_name]
  set ck_iterm [$inst findITerm "CK"]
  set net [$ck_iterm getNet]

  if { $net eq "NULL" } {
    puts "FAIL: $icg_name CK is disconnected"
    incr missing
    continue
  }

  set has_buf_driver 0
  foreach it [$net getITerms] {
    if { [$it isOutputSignal] } {
      set driver_name [[$it getInst] getName]
      if { [string match "clkbuf_*" $driver_name] } {
        set has_buf_driver 1
        break
      }
    }
  }

  if { !$has_buf_driver } {
    puts "FAIL: $icg_name CK on net [$net getName] has no CTS buffer driver"
    incr missing
  }
}

if { $missing == 0 } {
  puts "PASS: Both ICGs properly connected to CTS tree"
} else {
  puts "FAIL: $missing ICGs lost from CTS tree due to co-location"
}

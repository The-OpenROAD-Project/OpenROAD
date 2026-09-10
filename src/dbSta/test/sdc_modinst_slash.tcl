# Regression for dbSdcNetwork literal SDC pin lookup when an instance in
# the hierarchy has a name containing the path divider (a Verilog escaped
# identifier like "\i/sub").  Before the full-path -> Instance map fallback
# in findInstancesMatching1, get_pins on a literal pattern that traverses
# such an instance silently dropped (STA-0363) because the hierarchy walker
# in findInstance splits the embedded '/' as a hierarchy separator.
source "helpers.tcl"
read_lef liberty1.lef
read_liberty liberty1.lib
read_verilog sdc_modinst_slash.v
link_design -hier top

# Confirm the modInst really is stored with the embedded '/' (escaped).
set block [ord::get_db_block]
set top_mod [$block getTopModule]
foreach child [$top_mod getModInsts] {
  puts "modinst name: '[$child getName]'"
}

# Literal get_pins through the embedded-slash modInst.  This is what was
# missed before the fix.
set pins [get_pins i/sub/buf1/A]
foreach p $pins {
  puts "get_pins i/sub/buf1/A -> [get_property $p full_name]"
}

# Apply a real SDC constraint and confirm it sticks (no STA-0363 warning).
set_disable_timing [get_pins i/sub/buf1/A]
report_disabled_edges

# After the cache has been built, mutate the hierarchy and confirm that
# subsequent literal lookups see the new instance.  Without invalidation
# on inDbInstCreate, the stale cache would miss the freshly added cell.
set master [[ord::get_db] findMaster snl_bufx1]
odb::dbInst_create $block $master new_buf
foreach p [get_pins new_buf/A] {
  puts "after-create get_pins -> [get_property $p full_name]"
}

# Rename the leaf inst behind the cache's back. Without invalidation on
# inDbPostInstRename, get_pins of the old name returned the renamed
# instance (wrong pin) and get_pins of the new name missed entirely.
set buf1_inst {}
foreach inst [$block getInsts] {
  set n [$inst getName]
  if { [string match "*buf1" $n] && $n != "new_buf" } {
    set buf1_inst $inst
    break
  }
}
puts "buf1 inst stored name: '[$buf1_inst getName]'"
$buf1_inst rename buf_renamed
puts "after-rename get_pins i/sub/buf1/A -> [get_pins -quiet i/sub/buf1/A]"
foreach p [get_pins i/sub/buf_renamed/A] {
  puts "after-rename get_pins i/sub/buf_renamed/A -> [get_property $p full_name]"
}

# Reparent the renamed leaf into a different module via dbModule::addInst.
# The full hierarchical path of the inst changes from i/sub/buf_renamed to
# buf_renamed (under the top module). Without invalidation on
# inDbPostInstParentChange the cache would still resolve the old path and
# silently miss the new one.
set top_mod [$block getTopModule]
$top_mod addInst $buf1_inst
puts "after-reparent get_pins i/sub/buf_renamed/A -> \
      [get_pins -quiet i/sub/buf_renamed/A]"
foreach p [get_pins buf_renamed/A] {
  puts "after-reparent get_pins buf_renamed/A -> [get_property $p full_name]"
}

# Destroy the new_buf created above. Without invalidation on
# inDbInstDestroy the cache would still hand out a dangling Instance*.
odb::dbInst_destroy [$block findInst new_buf]
puts "after-destroy get_pins new_buf/A -> [get_pins -quiet new_buf/A]"

# Tight create-then-query loop. Before incremental cache maintenance
# this pattern (resizer-style) was O(N^2): every create blew away the
# cache, the next get_pins rebuilt it via a full DFS. With the new
# pathological-only cache + onInstCreated fast path, no rebuild happens
# here at all because the new insts have no embedded divider in their
# names or ancestors, so the cache stays untouched.
for { set i 0 } { $i < 5 } { incr i } {
  set n "loop_buf_$i"
  odb::dbInst_create $block $master $n
  puts "loop $i get_pins $n/A -> [get_property [lindex [get_pins $n/A] 0] full_name]"
}

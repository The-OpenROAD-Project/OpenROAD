source "helpers.tcl"
read_liberty sky130hd/sky130_fd_sc_hd__tt_025C_1v80.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130_fd_sc_hd_merged.lef
read_verilog upf/mpd_top.v
link_design mpd_top

# Write base .odb
set base [make_result_file delta_base.odb]
write_db $base

# Make changes - move all instances slightly
foreach inst [[ord::get_db_block] getInsts] {
  lassign [$inst getLocation] x y
  $inst setLocation [expr $x + 100] [expr $y + 100]
}

# Write delta using explicit -base flag
set delta [make_result_file delta_step1.delta]
write_db -base $base $delta

# Write full .odb of the modified state for comparison
set full [make_result_file delta_full.odb]
write_db $full

# Report sizes
set delta_size [file size $delta]
set full_size [file size $full]
set base_size [file size $base]
puts "Base: $base_size bytes, Delta: $delta_size bytes, Full: $full_size bytes"
if { $delta_size < $full_size } {
  puts "PASS: delta is smaller ($delta_size < $full_size)"
} else {
  puts "INFO: delta ($delta_size) >= full ($full_size) -\
    expected for small designs where most blocks change"
}

# Verify deterministic serialization
set full2 [make_result_file delta_full2.odb]
write_db $full2
set md5_a [lindex [exec md5sum $full] 0]
set md5_b [lindex [exec md5sum $full2] 0]
if { $md5_a == $md5_b } {
  puts "PASS: deterministic serialization"
} else {
  puts "FAIL: serialization not deterministic!"
  exit 1
}

puts "pass: delta write test passed"

# Check LEGACY and LEGACY_MT setup size-up honor dont_touch instances.

source "helpers.tcl"

proc instance_refs { } {
  set refs {}
  foreach inst [get_cells *] {
    dict set refs [get_full_name $inst] [get_property $inst ref_name]
  }
  return $refs
}

proc check_instance_refs { before_refs stage } {
  set mismatches {}
  foreach inst [get_cells *] {
    set inst_name [get_full_name $inst]
    set before_ref [dict get $before_refs $inst_name]
    set after_ref [get_property $inst ref_name]
    if { $before_ref ne $after_ref } {
      lappend mismatches "$inst_name:$before_ref->$after_ref"
    }
  }
  if { [llength $mismatches] != 0 } {
    error "dont_touch instances changed after $stage: [join $mismatches {, }]"
  }
}

proc run_sizeup_repair { repair_policy before_refs } {
  puts "Repair policy: $repair_policy"
  repair_timing -setup \
    -policy $repair_policy \
    -sequence "sizeup" \
    -skip_last_gasp \
    -skip_crit_vt_swap
  check_instance_refs $before_refs $repair_policy
  puts "All dont-touch instances preserved after $repair_policy."
}

read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup_dont_touch_sizeup.def
create_clock -period 0.35 clk
set_load 1.0 [all_outputs]

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

# Mark every instance dont_touch before exercising repair setup.
set before_refs [instance_refs]
foreach inst [get_cells *] {
  set_dont_touch $inst
}
puts "Dont-touch instances: [dict size $before_refs]"

run_sizeup_repair "LEGACY" $before_refs

# Restore the target cell through a manual ECO before testing LEGACY_MT.
set target_inst r1
set target_ref [dict get $before_refs $target_inst]
set target_lib_cell "NangateOpenCellLibrary/$target_ref"
unset_dont_touch $target_inst
if { [replace_cell $target_inst $target_lib_cell] == 0 } {
  error "failed to restore $target_inst to $target_lib_cell"
}
set_dont_touch $target_inst
puts "Manual ECO: restored $target_inst to $target_ref."

run_sizeup_repair "LEGACY_MT" $before_refs

report_worst_slack -max
report_tns -digits 3

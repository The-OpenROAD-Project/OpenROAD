# Check LEGACY setup size-up honors dont_touch instances.

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
    -phases $repair_policy \
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

report_worst_slack -max
report_tns -digits 3

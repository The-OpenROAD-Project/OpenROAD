read_lef $::env(TECH_LEF)
read_lef $::env(SC_LEF)
if {[info exist ::env(ADDITIONAL_LEFS)]} {
  foreach lef $::env(ADDITIONAL_LEFS) {
    read_lef $lef
  }
}

# Read liberty files
foreach libFile $::env(LIB_FILES) {
  read_liberty $libFile
}
# Read def and sdc
read_def $::env(RESULTS_DIR)/6_final.def

proc deleteNetByName {name} {
  set db [ord::get_db]
  set chip [$db getChip]
  set block [$chip getBlock]
  set net [$block findNet $name]
  $net destroySWires
  puts "\[INFO\] Deleted net '[$net getName]'"
}

deleteNetByName VDD
deleteNetByName VSS

write_def $::env(RESULTS_DIR)/6_final_no_power.def
exit

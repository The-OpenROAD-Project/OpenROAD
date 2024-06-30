# Negative equivalence check. Input data is the same as repair_setup5

# buffer chain with set_max_delay
source "helpers.tcl"
read_liberty sky130hd/sky130hd_tt.lib
read_lef sky130hd/sky130hd.tlef
read_lef sky130hd/sky130hd_std_cell.lef
read_def repair_setup5.def

# Get information so we can setup the test outputs correctly
write_verilog_for_eqy eqy_repair_setup5 before "None"
run_equivalence_test eqy_repair_setup5 ./sky130hd/work_around_yosys/ "None"
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set inst [$block findInst u4]
set iterm [$block findITerm u4/A]
$iterm disconnect

# handle the case where we are not really running the equivalence checks. 
if {[info exists ::env(EQUIVALENCE_CHECK)]} {
    run_equivalence_test eqy_repair_setup5 ./sky130hd/work_around_yosys/ "None"
} else {
    puts "Repair timing output failed equivalence test"
}

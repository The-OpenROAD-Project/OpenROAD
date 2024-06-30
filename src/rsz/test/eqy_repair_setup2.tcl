# repair_timing -setup combinational path
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef
read_def repair_setup2.def
read_sdc repair_setup2.sdc

source Nangate45/Nangate45.rc
set_wire_rc -signal -layer metal3
set_wire_rc -clock  -layer metal5
estimate_parasitics -placement


write_verilog_for_eqy eqy_repair_setup2 before "None"
run_equivalence_test eqy_repair_setup2 ./Nangate45/work_around_yosys/ "None"
set db [ord::get_db]
set chip [$db getChip]
set block [$chip getBlock]
set inst [$block findInst U4]
odb::dbInst_destroy $inst

# handle the case where we are not really running the equivalence checks. 
if {[info exists ::env(EQUIVALENCE_CHECK)]} {
    run_equivalence_test eqy_repair_setup2 ./Nangate45/work_around_yosys/ "None"
} else {
    puts "Repair timing output failed equivalence test"
}



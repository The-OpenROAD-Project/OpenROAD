# Generate wires over a range of lengths to observe buffering trends.
source "helpers.tcl"
read_liberty Nangate45/Nangate45_typ.lib
read_lef Nangate45/Nangate45.lef

set db [ord::get_db]
set tech [$db getTech]
set chip [odb::dbChip_create $db]
set block [odb::dbBlock_create $chip top]
$block setDefUnits 2000
# Tech dependent
set m1 [$tech findLayer metal1]
set buf [$db findMaster BUF_X1]
set buf_in [$buf findMTerm A]
set buf_out [$buf findMTerm Z]
for {set i 1} {$i <= 200} {incr i} {
    set n1 [odb::dbNet_create $block n1-$i]
    set n2 [odb::dbNet_create $block n2-$i]
    set n3 [odb::dbNet_create $block n3-$i]
    set bt1 [odb::dbBTerm_create $n1 p1-$i]
    set bt3 [odb::dbBTerm_create $n3 p3-$i]
    $bt1 setIoType INPUT
    $bt3 setIoType OUTPUT
    $bt1 connect $n1
    $bt3 connect $n3
    set pos [expr $i * 10 * 2000]
    set bp1 [odb::dbBPin_create $bt1]
    set bp3 [odb::dbBPin_create $bt3]
    odb::dbBox_create $bp1 $m1 0 $pos 100 [expr $pos + 100]
    odb::dbBox_create $bp3 $m1 $pos $pos [expr $pos + 100] [expr $pos + 100]
    $bp1 setPlacementStatus PLACED
    $bp3 setPlacementStatus PLACED
    set i1 [odb::dbInst_create $block $buf i1-$i]
    [$i1 getITerm $buf_in] connect $n1
    [$i1 getITerm $buf_out] connect $n2
    $i1 setPlacementStatus PLACED
    $i1 setLocation 0 $pos
    set i2 [odb::dbInst_create $block $buf i2-$i]
    [$i2 getITerm $buf_in] connect $n2
    [$i2 getITerm $buf_out] connect $n3
    $i2 setPlacementStatus PLACED
    $i2 setLocation $pos $pos
}
ord::design_created

create_clock -period 1 -name clk
set_input_delay -clock clk  1.0  [all_inputs]
set_output_delay -clock clk  0.0  [all_outputs]

set_wire_rc -layer metal2
estimate_parasitics -placement
repair_design
report_worst_slack -max -digits 3
report_tns -digits 3
repair_timing
report_worst_slack -max -digits 3
report_tns -digits 3
set def_file [make_result_file buffer_varying_lengths.def]
write_def $def_file
diff_files buffer_varying_lengths.defok $def_file

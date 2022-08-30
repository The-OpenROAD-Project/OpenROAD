source "helpers.tcl"


set db [ord::get_db]
read_lef "data/gscl45nm.lef"
read_def "data/design.def"
set chip [$db getChip]
set block [$chip getBlock]

set net [$block findNet inp0]
set inst [$block findInst _g2_]
set iterm [$block findITerm _g2_/A]
set bterm [$block findBTerm inp0]

$net setDoNotTouch true
catch {odb::dbNet_destroy $net}
catch {odb::dbBTerm_destroy $bterm}
catch {odb::dbBTerm_create $net name}
set test_inst [odb::dbInst_create $block [$inst getMaster] test]
set test_iterm [$block findITerm test/A]
catch {$test_iterm connect $net}
catch {$iterm disconnect}

$inst setDoNotTouch true
catch {odb::dbInst_destroy $inst}
catch {$iterm disconnect}
catch {$inst swapMaster [$db findMaster XNOR2X1]}


puts "pass"
exit

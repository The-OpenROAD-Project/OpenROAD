source "helpers.tcl"

# Test create_obstruction function
# This test creates various types of obstructions and verifies their properties

# Open database, load lef and design
set db [ord::get_db]
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "create_blockage.def"
set chip [$db getChip]
set block [$chip getBlock]

catch { create_obstruction -region {10 10 20 20} } err
puts $err

catch { create_obstruction -layer Metal1 -region {10 10 20 20} } err
puts $err

set b1 [create_obstruction -layer met1 -region {10 10 20 20}]
check "obstruction count" {llength [$block getObstructions]} 1

set b2 [create_obstruction -layer met1 -region {10 10 20 20} -slot -fill -except_pg]
check "obstruction count" {llength [$block getObstructions]} 2
check "obstruction is fill" {$b2 isFillObstruction} 1
check "obstruction is slot" {$b2 isSlotObstruction} 1
check "obstruction is except_pg" {$b2 isExceptPGNetsObstruction} 1

set b3 [create_obstruction -layer met1 -region {10 10 20 20} -min_spacing 1.5 -effective_width 0.5]
check "obstruction count" {llength [$block getObstructions]} 3
check "obstruction is eff width" {$b3 hasEffectiveWidth} 1
check "obstruction is spacing" {$b3 hasMinSpacing} 1
check "obstruction eff width" {$b3 getEffectiveWidth} 500
check "obstruction spacing" {$b3 getMinSpacing} 1500

set b4 [create_obstruction -layer met1 -region {70 70 80 80} -inst "_INST_"]
check "obstruction has instance" {[$b4 getInstance] getName} _INST_
check "obstruction count" {llength [$block getObstructions]} 4

# DEF test
set def_file [make_result_file create_obstruction.def]
write_def $def_file
diff_files create_obstruction.defok $def_file

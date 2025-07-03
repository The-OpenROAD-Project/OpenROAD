source "helpers.tcl"

# Test create_blockage function
# This test creates various types of blockages and verifies their properties

# Open database, load lef and design
set db [ord::get_db]
read_lef "sky130hd/sky130hd.tlef"
read_lef "sky130hd/sky130hd_std_cell.lef"
read_def "create_blockage.def"
set chip [$db getChip]
set block [$chip getBlock]

# Test 1: Create hard blockage
puts "Test 1: Creating hard blockage..."
set b1 [create_blockage -region {10 10 20 20}]
check "Blockage count" {llength [$block getBlockages]} 1

# Test 2: Create soft blockage
puts "Test 2: Creating soft blockage..."
set b2 [create_blockage -region {30 30 40 40} -soft]
check "Blockage count" {llength [$block getBlockages]} 2
check "Blockage is soft" {$b2 isSoft} 1

# Test 3: Create partial blockage with max density
puts "Test 3: Creating blockage with max density..."
set b3 [create_blockage -region {50 50 60 60} -max_density 50.0]
check "Blockage count" {llength [$block getBlockages]} 3
check "Blockage has max density" {$b3 getMaxDensity} 50.0
check "Density blockage is soft" {$b3 isSoft} 0

# Test 4: Create blockage with instance
puts "Test 4: Creating blockage with instance..."
set b4 [create_blockage -region {70 70 80 80} -inst "_INST_"]
check "Blockage has instance" {[$b4 getInstance] getName} _INST_
check "Blockage count" {llength [$block getBlockages]} 4

# Test 5: Test warning when using soft flag and max_density argument
puts "Test 5: Warning when creating a partial blockage with soft flag enabled"
set b5 [create_blockage -region {0 0 5 5} -soft -max_density 30]
check "Blockage count" {llength [$block getBlockages]} 5
check "Blockage has max density" {$b5 getMaxDensity} 30.0
check "Density blockage is soft" {$b5 isSoft} 0

# Test 6: Test error handling - invalid coordinates
puts "Test 6: Testing error handling..."
if {[catch {create_blockage -region {200 200 100 100}} msg]} {
    puts "Expected error caught: $msg"
    check "Invalid coordinates rejected" 1 [1]
} else {
    check "Invalid coordinates should have been rejected" 0 1
}

# Test 7: Test error handling - coordinates outside die area
if {[catch {create_blockage -region {1500 1500 1600 1600}} msg]} {
    puts "Expected error caught: $msg"
    check "Out of bounds coordinates rejected" 1 [1]
} else {
    check "Out of bounds coordinates should have been rejected" 0 1
}

# Test 8: Test error handling - invalid max density
if {[catch {create_blockage -region {10 10 20 20} -max_density 150} msg]} {
    puts "Expected error caught: $msg"
    check "Invalid max density rejected" 1 [1]
} else {
    check "Invalid max density should have been rejected" 0 1
}

# Test 9: Test error handling - non-existent instance
if {[catch {create_blockage -region {10 10 20 20} -inst "non_existent"} msg]} {
    puts "Expected error caught: $msg"
    check "Non-existent instance rejected" 1 [1]
} else {
    check "Non-existent instance should have been rejected" 0 1
}

# Verify final blockage count
set final_blockages [$block getBlockages]
check "Final blockage count" {llength $final_blockages} 5

# DEF test
set def_file [make_result_file create_blockage.def]
write_def $def_file
diff_files create_blockage.defok $def_file

puts "pass"
exit 0

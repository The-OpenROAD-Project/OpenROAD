source "helpers.tcl"

# Test set_halo function
# This test verifies halo geometry/soft-flag application and error handling

# Open database, load lef and design
set db [ord::get_db]
read_lef "Nangate45/Nangate45_tech.lef"
read_lef "Nangate45/Nangate45_stdcell.lef"
read_lef "Nangate45/fake_macros.lef"
read_def "set_halo.def"
set chip [$db getChip]
set block [$chip getBlock]

set macro1 [$block findInst "MACRO1"]
set macro2 [$block findInst "MACRO2"]
set buf1 [$block findInst "BUF1"]

# Test 1: Halo given as a single value applies uniformly to all four sides
puts "Test 1: Uniform halo (1 value)..."
set_halo -halo 1.0 -instance MACRO1
set h1 [$macro1 getHalo]
check "Uniform halo left" {$h1 xMin} 2000
check "Uniform halo bottom" {$h1 yMin} 2000
check "Uniform halo right" {$h1 xMax} 2000
check "Uniform halo top" {$h1 yMax} 2000
check "Uniform halo is hard by default" {$h1 isSoft} 0

# Test 2: Halo given as {width height}, plus -soft; also verifies that
# re-applying set_halo on the same instance replaces the previous halo
puts "Test 2: Width/height halo (2 values) with -soft..."
set_halo -halo { 1.0 2.0 } -instance MACRO1 -soft
set h2 [$macro1 getHalo]
check "Width/height halo left" {$h2 xMin} 2000
check "Width/height halo bottom" {$h2 yMin} 4000
check "Width/height halo right" {$h2 xMax} 2000
check "Width/height halo top" {$h2 yMax} 4000
check "Width/height halo is soft" {$h2 isSoft} 1

# Test 3: Halo given as {left bottom right top}, all distinct; also
# verifies that omitting -soft on a re-applied halo clears the soft flag
puts "Test 3: Explicit left/bottom/right/top halo (4 values)..."
set_halo -halo { 1.0 2.0 3.0 4.0 } -instance MACRO1
set h3 [$macro1 getHalo]
check "Explicit halo left" {$h3 xMin} 2000
check "Explicit halo bottom" {$h3 yMin} 4000
check "Explicit halo right" {$h3 xMax} 6000
check "Explicit halo top" {$h3 yMax} 8000
check "Explicit halo is hard again" {$h3 isSoft} 0

# Test 4: -apply_to_all_macros applies the halo to every macro instance,
# and does not create a halo on a non-macro (standard cell) instance
puts "Test 4: -apply_to_all_macros..."
set_halo -halo 0.5 -apply_to_all_macros
set h4a [$macro1 getHalo]
set h4b [$macro2 getHalo]
check "MACRO1 halo updated by apply_to_all_macros" {$h4a xMin} 1000
check "MACRO2 halo set by apply_to_all_macros" {$h4b xMin} 1000
check "Non-macro instance has no halo" {$buf1 getHalo} "NULL"

# Test 5: Error handling - missing required -halo argument
puts "Test 5: Testing error handling..."
if { [catch { set_halo -instance MACRO1 } msg] } {
  puts "Expected error caught: $msg"
  check "Missing -halo rejected" 1 [1]
} else {
  check "Missing -halo should have been rejected" 0 1
}

# Test 6: Error handling - halo list with an invalid number of values
if { [catch { set_halo -halo { 1.0 2.0 3.0 } -instance MACRO1 } msg] } {
  puts "Expected error caught: $msg"
  check "Invalid halo length rejected" 1 [1]
} else {
  check "Invalid halo length should have been rejected" 0 1
}

# Test 7: Error handling - negative halo value
if { [catch { set_halo -halo { -1.0 } -instance MACRO1 } msg] } {
  puts "Expected error caught: $msg"
  check "Negative halo value rejected" 1 [1]
} else {
  check "Negative halo value should have been rejected" 0 1
}

# Test 8: Error handling - neither -instance nor -apply_to_all_macros given
if { [catch { set_halo -halo 1.0 } msg] } {
  puts "Expected error caught: $msg"
  check "Missing -instance/-apply_to_all_macros rejected" 1 [1]
} else {
  check "Missing -instance/-apply_to_all_macros should have been rejected" 0 1
}

# Test 9: Error handling - non-existent instance
if { [catch { set_halo -halo 1.0 -instance "non_existent" } msg] } {
  puts "Expected error caught: $msg"
  check "Non-existent instance rejected" 1 [1]
} else {
  check "Non-existent instance should have been rejected" 0 1
}

# Test 10: Error handling - instance that is not a macro
if { [catch { set_halo -halo 1.0 -instance BUF1 } msg] } {
  puts "Expected error caught: $msg"
  check "Non-macro instance rejected" 1 [1]
} else {
  check "Non-macro instance should have been rejected" 0 1
}

# DEF test
set def_file [make_result_file set_halo.def]
write_def $def_file
diff_files set_halo.defok $def_file

puts "pass"
exit 0

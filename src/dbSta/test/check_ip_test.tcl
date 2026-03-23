# SPDX-License-Identifier: BSD-3-Clause
# Copyright (c) 2025, The OpenROAD Authors
#
# Test script for IP Checker (check_ip command)
# Tests all LEF checks LEF-CHK-001 through LEF-CHK-010 in ascending order
# Run from OpenROAD build directory:
#   ./bin/openroad -no_init ../src/dbSta/test/check_ip_test.tcl

puts "=============================================="
puts "IP Checker Test Suite"
puts "=============================================="

# Load the test LEF file
set script_dir [file dirname [info script]]
set test_lef "$script_dir/check_ip_test.lef"
set test_def "$script_dir/check_ip_test.def"
puts "Loading: $test_lef"
read_lef $test_lef

# Load DEF to set up track grids (needed for LEF-CHK-003)
puts "Loading: $test_def (track grid definitions)"
read_def $test_def

# Helper to run a test that expects failure
proc test_check_fails { test_name master_name { extra_args "" } } {
  puts "\n--- Test: $test_name ---"
  puts "Testing master: $master_name"
  set cmd "check_ip -master $master_name $extra_args"
  puts "Command: $cmd"
  if { [catch { eval { $cmd } } err] } {
    puts "EXPECTED FAILURE: $err"
    return 1
  } else {
    puts "UNEXPECTED PASS - this test should have failed!"
    return 0
  }
}

# Helper to run a test that expects success
proc test_check_passes { test_name master_name { extra_args "" } } {
  puts "\n--- Test: $test_name ---"
  puts "Testing master: $master_name"
  set cmd "check_ip -master $master_name $extra_args"
  puts "Command: $cmd"
  if { [catch { eval { $cmd } } err] } {
    puts "UNEXPECTED FAILURE: $err"
    return 0
  } else {
    puts "PASS: No warnings"
    return 1
  }
}

puts "\n=============================================="
puts "Running Individual Tests"
puts "=============================================="

# Test 0: Passing test (all checks pass)
test_check_passes "LEF-CHK-000: All checks pass" "pass_all_checks"

# Test 1: LEF-CHK-001 - Macro dimensions not on manufacturing grid
test_check_fails "LEF-CHK-001: Macro width not aligned to mfg grid" "lef001_grid_width"

# Test 2: LEF-CHK-002 - Pin coordinates not on manufacturing grid
test_check_fails "LEF-CHK-002: Pin coords not aligned to mfg grid" "lef002_pin_grid"

# Test 3a: LEF-CHK-003 - Single-pattern pass (pin distances are multiples of pitch)
test_check_passes "LEF-CHK-003a: Single-pattern compatible (PASS)" "lef003a_single_pass"

# Test 3b: LEF-CHK-003 - Single-pattern fail (pin distance GCD not multiple of pitch)
test_check_fails "LEF-CHK-003b: Single-pattern incompatible (FAIL)" "lef003b_single_fail"

# Test 3c: LEF-CHK-003 - Multi-pattern pass (effective pitch from two track patterns)
test_check_passes "LEF-CHK-003c: Multi-pattern effective pitch (PASS)" "lef003c_multi_pass"

# Test 3d: LEF-CHK-003 - Multi-pattern fail (still can't align with effective pitch)
test_check_fails "LEF-CHK-003d: Multi-pattern still incompatible (FAIL)" "lef003d_multi_fail"

# Test 4-5a: LEF-CHK-004-005 - Pin blocked on M1 edges AND M2 above (FAIL)
test_check_fails "LEF-CHK-004-005a: Pin fully blocked (M1+M2)" "lef004_005a_fully_blocked"

# Test 4-5b: LEF-CHK-004-005 - Pin blocked on M1 but accessible from M2 above (PASS)
test_check_passes \
  "LEF-CHK-004-005b: Blocked on M1 but from M2 (PASS)" "lef004_005b_multi_shape_pass"

# Test 4-5c: LEF-CHK-004-005 - Power pin blocked on M1+M2 (FAIL)
test_check_fails "LEF-CHK-004-005c: Power pin blocked (M1+M2)" "lef004_005c_power_blocked"

# Test 4-5d: LEF-CHK-004-005 - Pin on macro boundary covered by OBS (should PASS)
# Pin at x=0 extends to macro edge, accessible from outside even though covered by OBS
test_check_passes "LEF-CHK-004-005d: Edge pin accessible from outside (PASS)" "lef004_005d_edge_pin"

# Test 6a: LEF-CHK-006 - Excessive polygon count (below threshold = FAIL)
test_check_fails \
  "LEF-CHK-006: Excessive polygon count (threshold 5)" "lef006_polygon_count" "-max_polygons 5"

# Test 6b: LEF-CHK-006 - Polygon count at threshold (should PASS)
test_check_passes \
  "LEF-CHK-006: Polygon count at threshold (threshold 6)" "lef006_polygon_count" "-max_polygons 6"

# Test 7: LEF-CHK-007 - Missing antenna info
test_check_fails "LEF-CHK-007: Missing antenna model" "lef007_no_antenna"

# Test 8: LEF-CHK-008 - FinFET detection (info only, requires -verbose)
# This is an INFO message, not a warning, so it should pass
test_check_passes "LEF-CHK-008: FinFET detection (verbose)" "lef008_finfet" "-verbose"

# Test 9: LEF-CHK-009 - Missing pin geometry
test_check_fails "LEF-CHK-009: Pin has no geometry" "lef009_no_geometry"

# Test 10a: LEF-CHK-010a - Pin width perpendicular to direction is too small (FAIL)
test_check_fails "LEF-CHK-010a: Pin width too small perpendicular to routing" "lef010a_small_width"

# Test 10b: LEF-CHK-010b - Pin width OK perpendicular, small in parallel (PASS)
test_check_passes "LEF-CHK-010b: Pin length small but width OK (PASS)" "lef010b_length_ok"

# Test 10c: LEF-CHK-010b - Pin area less than layer min area (FAIL)
test_check_fails "LEF-CHK-010b: Pin area too small" "lef010b_small_area"

# Test 10d: LEF-CHK-010b - Pin area meets layer min area (PASS)
test_check_passes "LEF-CHK-010b: Pin area OK (PASS)" "lef010b_area_ok"

puts "\n=============================================="
puts "IP Checker Test Suite Complete"
puts "=============================================="

exit

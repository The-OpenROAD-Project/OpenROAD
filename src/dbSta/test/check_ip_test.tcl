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
puts "Loading: $test_lef"
read_lef $test_lef

# Helper to run a test that expects failure
proc test_check_fails {test_name master_name {extra_args ""}} {
    puts "\n--- Test: $test_name ---"
    puts "Testing master: $master_name"
    if {$extra_args ne ""} {
        set cmd "check_ip -master $master_name $extra_args"
    } else {
        set cmd "check_ip -master $master_name"
    }
    puts "Command: $cmd"
    if {[catch {eval $cmd} err]} {
        puts "EXPECTED FAILURE: $err"
        return 1
    } else {
        puts "UNEXPECTED PASS - this test should have failed!"
        return 0
    }
}

# Helper to run a test that expects success
proc test_check_passes {test_name master_name {extra_args ""}} {
    puts "\n--- Test: $test_name ---"
    puts "Testing master: $master_name"
    if {$extra_args ne ""} {
        set cmd "check_ip -master $master_name $extra_args"
    } else {
        set cmd "check_ip -master $master_name"
    }
    puts "Command: $cmd"
    if {[catch {eval $cmd} err]} {
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

# Test 3: LEF-CHK-003 - Pin not aligned to routing grid
# NOTE: This check requires track grids which need a full design setup.
# Skipping as it requires external PDK files.
puts "\n--- Test: LEF-CHK-003: Pin not aligned to routing grid ---"
puts "SKIPPED: Requires track grids (needs full design setup with PDK)"

# Test 4: LEF-CHK-004 - Signal pin not accessible
test_check_fails "LEF-CHK-004: Signal pin blocked by obstruction" "lef004_signal_blocked"

# Test 5: LEF-CHK-005 - Power pin not accessible
test_check_fails "LEF-CHK-005: Power pin blocked by obstruction" "lef005_power_blocked"

# Test 6: LEF-CHK-006 - Excessive polygon count (use low threshold)
test_check_fails "LEF-CHK-006: Excessive polygon count" "lef006_polygon_count" "-max_polygons 5"

# Test 7: LEF-CHK-007 - Missing antenna info
test_check_fails "LEF-CHK-007: Missing antenna model" "lef007_no_antenna"

# Test 8: LEF-CHK-008 - FinFET detection (info only, requires -verbose)
# This is an INFO message, not a warning, so it should pass
puts "\n--- Test: LEF-CHK-008: FinFET detection (info only) ---"
puts "Testing with -verbose flag on lef008_finfet..."
test_check_passes "LEF-CHK-008: FinFET detection (verbose)" "lef008_finfet" "-verbose"

# Test 9: LEF-CHK-009 - Missing pin geometry
test_check_fails "LEF-CHK-009: Pin has no geometry" "lef009_no_geometry"

# Test 10: LEF-CHK-010 - Pin smaller than layer minimum
test_check_fails "LEF-CHK-010: Pin geometry too small" "lef010_small_pin"

puts "\n=============================================="
puts "IP Checker Test Suite Complete"
puts "=============================================="

exit

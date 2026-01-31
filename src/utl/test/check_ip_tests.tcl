# Comprehensive Test Suite for IP Checker (check_ip command)
# This test covers all LEF checks
#
# LEF Checks covered:
# LEF-001: Macro dimensions aligned to manufacturing grid
# LEF-002: Pin coordinates aligned to manufacturing grid  
# LEF-003: Pin routing grid alignment
# LEF-004: Signal pin accessibility (not obstructed)
# LEF-005: Power pin accessibility (not obstructed)
# LEF-006: Excessive polygon count
# LEF-007: Antenna information present
# LEF-008: FinFET fin grid property (informational)
# LEF-009: Pin geometry presence
# LEF-010: Pin minimum dimensions

# ============================================================================
# Step 1: Generate comprehensive LEF file with test macros
# ============================================================================

set lef_file "check_ip_test.lef"
set out [open $lef_file w]

# LEF Header
puts $out "VERSION 5.8 ;"
puts $out "BUSBITCHARS \"\[\]\" ;"
puts $out "DIVIDERCHAR \"/\" ;"
puts $out ""
puts $out "UNITS"
puts $out "  DATABASE MICRONS 1000 ;"
puts $out "END UNITS"
puts $out ""
puts $out "MANUFACTURINGGRID 0.005 ;"
puts $out ""

# Define routing layer M1
puts $out "LAYER M1"
puts $out "  TYPE ROUTING ;"
puts $out "  DIRECTION HORIZONTAL ;"
puts $out "  PITCH 0.200 ;"
puts $out "  WIDTH 0.100 ;"
puts $out "END M1"
puts $out ""

# Define a FinFET layer for LEF-008 detection
puts $out "LAYER FIN"
puts $out "  TYPE ROUTING ;"
puts $out "  DIRECTION VERTICAL ;"
puts $out "  PITCH 0.054 ;"
puts $out "  WIDTH 0.027 ;"
puts $out "END FIN"
puts $out ""

# Define SITE for floorplan
puts $out "SITE unit"
puts $out "  CLASS CORE ;"
puts $out "  SIZE 0.200 BY 2.000 ;"
puts $out "  SYMMETRY Y ;"
puts $out "END unit"
puts $out ""

# ============================================================================
# MACRO 1: grid_misaligned - Tests LEF-001 and LEF-002
# ============================================================================
puts $out "MACRO grid_misaligned"
puts $out "  CLASS BLOCK ;"
puts $out "  ORIGIN 0 0 ;"
puts $out "  SIZE 10.001 BY 10.000 ;"  ;# Width not on 0.005 grid -> LEF-001
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      RECT 0.001 0.000 0.100 0.100 ;"  ;# xMin off grid -> LEF-002
puts $out "    END"
puts $out "  END A"
puts $out "END grid_misaligned"
puts $out ""

# ============================================================================
# MACRO 2: signal_obstructed - Tests LEF-004
# ============================================================================
puts $out "MACRO signal_obstructed"
puts $out "  CLASS BLOCK ;"
puts $out "  ORIGIN 0 0 ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN SIG"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      RECT 1.000 1.000 1.200 1.200 ;"
puts $out "    END"
puts $out "  END SIG"
puts $out "  OBS"
puts $out "    LAYER M1 ;"
puts $out "    RECT 0.900 0.900 1.300 1.300 ;"  ;# Overlaps signal pin -> LEF-004
puts $out "  END"
puts $out "END signal_obstructed"
puts $out ""

# ============================================================================
# MACRO 3: power_obstructed - Tests LEF-005
# ============================================================================
puts $out "MACRO power_obstructed"
puts $out "  CLASS BLOCK ;"
puts $out "  ORIGIN 0 0 ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN VDD"
puts $out "    DIRECTION INOUT ;"
puts $out "    USE POWER ;"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      RECT 2.000 2.000 2.200 2.200 ;"
puts $out "    END"
puts $out "  END VDD"
puts $out "  OBS"
puts $out "    LAYER M1 ;"
puts $out "    RECT 1.900 1.900 2.300 2.300 ;"  ;# Fully covers VDD -> LEF-005
puts $out "  END"
puts $out "END power_obstructed"
puts $out ""

# ============================================================================
# MACRO 4: no_geometry - Tests LEF-009
# ============================================================================
puts $out "MACRO no_geometry"
puts $out "  CLASS BLOCK ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"
puts $out "    # No PORT section -> LEF-009"
puts $out "  END A"
puts $out "END no_geometry"
puts $out ""

# ============================================================================
# MACRO 5: no_antenna - Tests LEF-007
# ============================================================================
puts $out "MACRO no_antenna"
puts $out "  CLASS BLOCK ;"
puts $out "  ORIGIN 0 0 ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    # No ANTENNAMODEL -> LEF-007"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      RECT 1.000 1.000 1.200 1.200 ;"
puts $out "    END"
puts $out "  END A"
puts $out "END no_antenna"
puts $out ""

# ============================================================================
# MACRO 6: routing_misaligned - Tests LEF-003
# ============================================================================
puts $out "MACRO routing_misaligned"
puts $out "  CLASS BLOCK ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      # Pin center Y = 0.550, tracks at 0.0, 0.2, 0.4, 0.6..."
puts $out "      # 0.550 is not on track -> LEF-003"
puts $out "      RECT 1.000 0.500 1.200 0.600 ;"
puts $out "    END"
puts $out "  END A"
puts $out "END routing_misaligned"
puts $out ""

# ============================================================================
# MACRO 7: min_width_violation - Tests LEF-010
# ============================================================================
puts $out "MACRO min_width_violation"
puts $out "  CLASS BLOCK ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      # M1 min width = 0.100, this is 0.050 wide -> LEF-010"
puts $out "      RECT 1.000 1.000 1.050 2.000 ;"
puts $out "    END"
puts $out "  END A"
puts $out "END min_width_violation"
puts $out ""

# ============================================================================
# MACRO 8: many_polygons - Tests LEF-006 (when threshold is low)
# ============================================================================
puts $out "MACRO many_polygons"
puts $out "  CLASS BLOCK ;"
puts $out "  SIZE 10.000 BY 10.000 ;"
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      RECT 1.000 0.950 1.200 1.050 ;"
puts $out "    END"
puts $out "  END A"
puts $out "  OBS"
puts $out "    LAYER M1 ;"
# Generate many obstruction rectangles
for {set i 0} {$i < 10} {incr i} {
  set x [expr {3.0 + $i * 0.2}]
  puts $out "    RECT $x 3.000 [expr {$x + 0.100}] 3.100 ;"
}
puts $out "  END"
puts $out "END many_polygons"
puts $out ""

# ============================================================================
# MACRO 9: clean_macro - Should pass all checks
# ============================================================================
puts $out "MACRO clean_macro"
puts $out "  CLASS BLOCK ;"
puts $out "  SIZE 10.000 BY 10.000 ;"  ;# On grid
puts $out "  PIN A"
puts $out "    DIRECTION INPUT ;"
puts $out "    USE SIGNAL ;"
puts $out "    ANTENNAMODEL OXIDE1 ;"  ;# Has antenna
puts $out "    PORT"
puts $out "      LAYER M1 ;"
puts $out "      # On mfg grid, on routing track (Y center = 1.0)"
puts $out "      RECT 1.000 0.950 1.200 1.050 ;"
puts $out "    END"
puts $out "  END A"
puts $out "END clean_macro"
puts $out ""

puts $out "END LIBRARY"
close $out

# ============================================================================
# Step 2: Create dummy Verilog for design linking
# ============================================================================

set verilog_file "check_ip_test.v"
set out [open $verilog_file w]
puts $out "module top();"
puts $out "endmodule"
close $out

# ============================================================================
# Step 3: Load LEF and setup design
# ============================================================================

read_lef $lef_file
read_verilog $verilog_file
link_design top

# Initialize floorplan to enable routing grid checks
initialize_floorplan -die_area "0 0 100 100" -core_area "10 10 90 90" -site "unit"

# Create tracks for M1 (Horizontal layer with Y tracks at 0.0, 0.2, 0.4, ...)
make_tracks M1 -x_pitch 0.200 -x_offset 0.0 -y_pitch 0.200 -y_offset 0.0

# ============================================================================
# Step 4: Run individual macro checks
# ============================================================================

puts "\n============================================================================"
puts "IP CHECKER TEST SUITE"
puts "============================================================================\n"

# Test 1: LEF-001, LEF-002 - Grid alignment
puts ">>> Test 1: grid_misaligned (Expect LEF-001, LEF-002)"
catch {check_ip -master grid_misaligned} result
puts "Result: $result\n"

# Test 2: LEF-004 - Signal pin obstruction
puts ">>> Test 2: signal_obstructed (Expect LEF-004)"
catch {check_ip -master signal_obstructed} result
puts "Result: $result\n"

# Test 3: LEF-005 - Power pin obstruction
puts ">>> Test 3: power_obstructed (Expect LEF-005)"
catch {check_ip -master power_obstructed} result
puts "Result: $result\n"

# Test 4: LEF-009 - Missing pin geometry
puts ">>> Test 4: no_geometry (Expect LEF-009)"
catch {check_ip -master no_geometry} result
puts "Result: $result\n"

# Test 5: LEF-007 - Missing antenna model
puts ">>> Test 5: no_antenna (Expect LEF-007)"
catch {check_ip -master no_antenna} result
puts "Result: $result\n"

# Test 6: LEF-003 - Routing grid misalignment
puts ">>> Test 6: routing_misaligned (Expect LEF-003)"
catch {check_ip -master routing_misaligned} result
puts "Result: $result\n"

# Test 7: LEF-010 - Minimum width violation
puts ">>> Test 7: min_width_violation (Expect LEF-010)"
catch {check_ip -master min_width_violation} result
puts "Result: $result\n"

# Test 8: LEF-006 - Polygon count (with low threshold)
puts ">>> Test 8: many_polygons with -max_polygons 5 (Expect LEF-006)"
catch {check_ip -master many_polygons -max_polygons 5} result
puts "Result: $result\n"

# Test 9: Clean macro should pass
puts ">>> Test 9: clean_macro (Expect PASS)"
if { [catch {check_ip -master clean_macro} result] } {
  puts "FAIL: clean_macro failed check: $result"
} else {
  puts "PASS: clean_macro passed all checks"
}
puts ""

# Test 10: Check all macros
puts ">>> Test 10: Check ALL macros (Expect multiple warnings)"
catch {check_ip -all} result
puts "Final result: $result\n"

# Test 11: Verbose mode with FinFET detection
puts ">>> Test 11: Verbose mode on clean_macro (Check for LEF-008 info)"
if { [catch {check_ip -master clean_macro -verbose} result] } {
  puts "Result: $result"
} else {
  puts "Verbose check completed"
}
puts ""

# Test 12: Report file generation
puts ">>> Test 12: Generate report file"
catch {check_ip -all -report_file check_ip_report.txt}
if { [file exists check_ip_report.txt] } {
  puts "PASS: Report file generated"
  puts "Report contents:"
  puts "----------------"
  set fp [open check_ip_report.txt r]
  puts [read $fp]
  close $fp
} else {
  puts "FAIL: Report file not generated"
}

puts "\n============================================================================"
puts "TEST SUITE COMPLETE"
puts "============================================================================\n"

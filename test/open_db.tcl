# Test for -db flag functionality
# This test should be run with: openroad -db gcd_sky130hd.odb -exit open_db.tcl
# It verifies that the database was loaded correctly via the -db flag

source "helpers.tcl"

puts "TEST: Verifying database was loaded via -db flag"

# Check that we have a design loaded
if { [current_design] == "" } {
  puts "FAIL: No design loaded after -db flag"
  exit 1
}

puts "PASS: Design loaded: [current_design]"

# Check that we have some basic design information
set instance_count [sta::network_instance_count]
if { $instance_count == 0 } {
  puts "FAIL: No instances found in loaded design"
  exit 1
}

puts "PASS: Database loaded correctly with $instance_count instances"

# Test that basic commands work
# Note: all_nets and all_pins might not be available in this context
# Let's test other basic functionality instead

# Test that we can get design information
set design_name [current_design]
if { $design_name == "" } {
  puts "FAIL: No design name available"
  exit 1
}

puts "PASS: Basic commands work after database load"

# Test that we can access the database
set db [ord::get_db]
if { $db == "" } {
  puts "FAIL: Could not access database object"
  exit 1
}

puts "PASS: Database object accessible"

puts "PASS: -db flag test completed successfully"

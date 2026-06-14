# Test case for segfault on repair_timing issue #9768
# This test reproduces a segfault in sta::Table::findValue during delay calculation
# when repair_timing is called on certain circuit configurations.
#
# Issue: https://github.com/The-OpenROAD-Project/OpenROAD/issues/9768
# Error: Assertion '__n < this->size()' failed in stl_vector.h 
# Stack: sta::Table::findValue -> sta::GateTableModel::gateDelay -> repair_timing
#
# This test verifies that repair_timing completes without crashing.
# The actual test case files are available at:
# https://github.com/The-OpenROAD-Project/OpenROAD/issues/9768

# Note: Full test requires circuit files from issue #9768
# This is a placeholder to track the bug and ensure fix validation

puts "Repair timing segfault test - requires test case files from issue #9768"

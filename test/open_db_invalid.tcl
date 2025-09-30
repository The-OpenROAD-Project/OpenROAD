# Test for -db flag with invalid database file
# This test verifies proper error handling when an invalid database file is specified
# Expected behavior: OpenROAD should exit with error message and not reach this script

source "helpers.tcl"

puts "TEST: Testing -db flag with invalid database file"

# This test should be run with: openroad -db /nonexistent/file.odb -exit open_db_invalid.tcl
# The -db flag should be processed before this script runs
# Expected behavior: OpenROAD should exit with error before reaching this script


# Check the requirement for a placed VDD boundary pin on each chiplet.
# Chip B has no placed VDD boundary pin; its PDN and bump mapping remain connected.

source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_test_data/3dic_check_power_grid_require_bterms_fail.3dbx

puts "\nCommand - check_power_grid -net VDD -dont_require_terminals"
check_power_grid -net VDD -dont_require_terminals

puts "\nCommand - check_power_grid -net VDD"
catch { check_power_grid -net VDD } err
puts $err

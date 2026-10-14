# Check the requirement for a placed VDD boundary pin on each chiplet.
# Chip B has no placed VDD boundary pin; its PDN and bump mapping remain connected.

source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_check_power_grid_require_bterms_fail.3dbx

# Rules for wiring inside both Nangate45 chiplets.
set_extraction_rules_file -tech Nangate45_tech ../../../test/Nangate45/Nangate45.rcx_rules

# Rules for the bonds between chiplets.
set_extraction_rules_file -assembly 3dic_frontside_assembly.rules

# Extract parasitics
extract_parasitics

puts "\nCommand - check_power_grid -net VDD -dont_require_terminals"
check_power_grid -net VDD -dont_require_terminals

puts "\nCommand - check_power_grid -net VDD"
catch { check_power_grid -net VDD } err
puts $err

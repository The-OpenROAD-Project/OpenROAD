source "helpers.tcl"

# Load the AES and GCD chiplets and their face-to-face power bonds.
read_3dbx 3dic_test_data/3dic_check_power_grid_aes_gcd_nangate45.3dbx

# Read the resistance used for each inter-chip bond.
set_extraction_rules_file -assembly 3dic_test_data/3dic_frontside_assembly.rules

# Verify four VDD bonds and two VSS bonds connect both chiplet PDNs.
check_power_grid -net VDD -dont_require_terminals
check_power_grid -net VSS -dont_require_terminals

# Supply chipA from the package and analyze both connected VDD grids.
analyze_power_grid -net VDD -vsrc 3dic_test_data/3dic_check_power_grid_aes_gcd_nangate45_vdd.loc

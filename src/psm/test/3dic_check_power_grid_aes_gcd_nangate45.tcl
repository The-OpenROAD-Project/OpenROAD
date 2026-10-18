source "helpers.tcl"

# Load the AES and GCD chiplets and their face-to-face power bonds.
read_3dbx 3dic_test_data/3dic_check_power_grid_aes_gcd_nangate45.3dbx

# Set a custom test resistance for each inter-chip bond.
# Ideally, this value should come from the PDK.
set_psm_interchip_resistance -resistance 0.1

# Verify four VDD bonds and two VSS bonds connect both chiplet PDNs.
check_power_grid -net VDD -dont_require_terminals
check_power_grid -net VSS -dont_require_terminals

# Supply chipA from the package and analyze both connected VDD grids.
analyze_power_grid -net VDD -vsrc 3dic_test_data/3dic_check_power_grid_aes_gcd_nangate45_vdd.loc

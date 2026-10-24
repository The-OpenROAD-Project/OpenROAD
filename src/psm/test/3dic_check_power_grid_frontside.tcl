# Check 3D IC connectivity for VDD and VSS net
source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_test_data/3dic_frontside.3dbx

# Set a custom test resistance for each inter-chip bond.
# Ideally, this value should come from the PDK.
set_psm_interchip_resistance -resistance 0.1

check_power_grid -net VDD
check_power_grid -net VSS

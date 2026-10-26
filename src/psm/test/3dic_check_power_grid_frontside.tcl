# Check 3D IC connectivity for VDD and VSS net
source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_test_data/3dic_frontside.3dbx

check_power_grid -net VDD
check_power_grid -net VSS

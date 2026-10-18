# In 3D IC check whether VDD check fails when Chip B has no VDD bump mapping

source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_test_data/3dic_check_power_grid_missing_bump_mapping.3dbx

# Set a custom test resistance for each inter-chip bond.
# Ideally, this value should come from the PDK.
# No resistor is created because Chip B has no mapped VDD bump.
set_psm_interchip_resistance -resistance 0.1

catch { check_power_grid -net VDD } err
puts $err

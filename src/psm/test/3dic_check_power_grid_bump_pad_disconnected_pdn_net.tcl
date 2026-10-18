# In a 3D IC check that if a VDD bump PAD is disconnected from Chip B's PDN.
# then error occurs

source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_test_data/3dic_check_power_grid_bump_pad_disconnected_pdn_net.3dbx

# Set a custom test resistance for each inter-chip bond.
# Ideally, this value should come from the PDK.
set_psm_interchip_resistance -resistance 0.1

catch { check_power_grid -net VDD } err
puts $err

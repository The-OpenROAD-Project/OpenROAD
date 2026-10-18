# Check 3D VDD analysis, IR/EM reports, and the saved instance voltages.
source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_test_data/3dic_frontside.3dbx

# Read the timing constraints for this custom design.
read_sdc 3dic_test_data/3dic_fake.sdc

# Set a custom test resistance for each inter-chip bond.
# Ideally, this value should come from the PDK.
set_psm_interchip_resistance -resistance 0.1

set voltage_file [make_result_file 3dic_test_vdd.rpt]

check_power_grid -net VDD

# Apply a 1.1 V supply at Chip A's VDD port.
set_pdnsim_net_voltage -net VDD -chiplet chipA -port VDD -voltage 1.1

analyze_power_grid -net VDD -voltage_file $voltage_file -enable_em

diff_files $voltage_file 3dic_test_vdd.rptok

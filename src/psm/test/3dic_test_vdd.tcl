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

# Apply 1.1 V sources to Chip A using their physical locations.
analyze_power_grid -net VDD \
  -vsrc 3dic_test_data/3dic_external_power_source.loc \
  -voltage_file $voltage_file -enable_em

diff_files $voltage_file 3dic_test_vdd.rptok

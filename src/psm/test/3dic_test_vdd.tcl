# Check 3D VDD analysis, IR/EM reports, and the saved instance voltages.
source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_frontside.3dbx

# Read the timing constraints for this custom design.
read_sdc 3dic_fake.sdc

# Rules for wiring inside both Nangate45 chiplets.
set_extraction_rules_file -tech Nangate45_tech ../../../test/Nangate45/Nangate45.rcx_rules

# Rules for the bonds between chiplets.
set_extraction_rules_file -assembly 3dic_frontside_assembly.rules

# Extract parasitics
extract_parasitics

set voltage_file [make_result_file 3dic_test_vdd.rpt]

check_power_grid -net VDD

# Apply a 1.1 V supply at Chip A's VDD port.
set_pdnsim_net_voltage -net VDD -chiplet chipA -port VDD -voltage 1.1

analyze_power_grid -net VDD -voltage_file $voltage_file -enable_em

diff_files $voltage_file 3dic_test_vdd.rptok

# Verify that a VDD source on chipA powers both connected chiplets.
source "helpers.tcl"

read_3dbx 3dic_test_data/3dic_frontside.3dbx
read_sdc 3dic_test_data/3dic_fake.sdc

set_extraction_rules_file -assembly 3dic_test_data/3dic_frontside_assembly.rules

check_power_grid -net VDD

set voltage_file [make_result_file 3dic_external_power_source.rpt]

analyze_power_grid -net VDD -vsrc 3dic_test_data/3dic_external_power_source.loc \
  -voltage_file $voltage_file

diff_files $voltage_file 3dic_external_power_source.rptok

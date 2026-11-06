# Verify that each scene uses its configured inter-chip resistance.
source "helpers.tcl"

read_3dbx 3dic_test_data/3dic_frontside.3dbx
read_sdc 3dic_test_data/3dic_fake.sdc

define_scene low_resistance -liberty NangateOpenCellLibrary
define_scene high_resistance -liberty NangateOpenCellLibrary

set_psm_interchip_resistance -scene low_resistance -resistance 0.1
set_psm_interchip_resistance -scene high_resistance -resistance 1000.0

check_power_grid -net VDD

analyze_power_grid -net VDD -corner low_resistance \
  -vsrc 3dic_test_data/3dic_external_power_source.loc

analyze_power_grid -net VDD -corner high_resistance \
  -vsrc 3dic_test_data/3dic_external_power_source.loc

analyze_power_grid -net VDD -corner low_resistance \
  -vsrc 3dic_test_data/3dic_external_power_source.loc -allow_reuse

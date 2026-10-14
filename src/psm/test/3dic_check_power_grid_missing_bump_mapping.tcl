# In 3D IC check whether VDD check fails when Chip B has no VDD bump mapping

source "helpers.tcl"

# Load the two chiplets, their layouts, bumps, and connections.
read_3dbx 3dic_check_power_grid_missing_bump_mapping.3dbx

# Rules for wiring inside both Nangate45 chiplets.
set_extraction_rules_file -tech Nangate45_tech ../../../test/Nangate45/Nangate45.rcx_rules

# Rules for the bonds between chiplets.
set_extraction_rules_file -assembly 3dic_frontside_assembly.rules

# Extract parasitics
extract_parasitics

catch { check_power_grid -net VDD } err
puts $err

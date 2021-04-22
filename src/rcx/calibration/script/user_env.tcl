############################
# DATA GENERATION SETTINGS 
############################

# Variables that point to the LEF files
# NEEDS to be changed for different tech.
set DIR ../../test/sky130
set TECH_LEF ${DIR}/sky130_tech.lef
set MACRO_LEF ${DIR}/sky130_std_cell.lef

# This is just an example of the parasitics of the 
# patterns that is used for Demo purposes.
# NEEDS to be changed for different tech.
set golden_spef ../../test/generate_pattern.spefok

# Technology Node 
set PROCESS_NODE 130

# Process corner
# NEEDS to be changed for different corners.
set CORNER RCmax

# The file name and location of the custom RC tech file
# The name is subject to change
set extRules ../data/${CORNER}.rules

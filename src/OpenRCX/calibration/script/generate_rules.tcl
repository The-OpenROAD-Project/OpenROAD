#################################################
# Desc: This script is used to create the RC 
#       table used for OpenRCX parasitic 
#       calculation. It takes the patterns layout
#       parasitics from reference extractor (SPEF)
#       and convert it to Extraction
#       Rules file (RC Table).
#       
# Input:  - tech_Lef
#         - patterns.spef
#
# Output: - <extRules>.rules
#
##################################################

source ../script/user_env.tcl

read_lef $TECH_LEF

# Read the patterns design
read_def EXT/patterns.def

# Read the parasitics of the patterns
bench_read_spef $golden_spef

# Convert the parasitics into 
write_rules -file $extRules -db

exit

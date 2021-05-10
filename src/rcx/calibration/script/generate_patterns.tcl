#################################################
# Desc: This script is used to generate patterns 
#       geometries that model various capacitance 
#       and resistance models
# Input: tech_Lef
# Output: patterns.def
#         patterns.v
#################################################

source ../script/user_env.tcl

set ext_dir EXT
exec mkdir -p $ext_dir

# Read Technology LEF
read_lef $TECH_LEF

# Creates the patterns and 
# store it in the database
bench_wires -len 100 -all

# Writes the verilog netlist 
# of the patterns
bench_verilog $ext_dir/patterns.v

write_def $ext_dir/patterns.def

exit

set DIR ../../test

# Read LEF and DEF
read_lef ${DIR}/Nangate45/Nangate45.lef

read_def -order_wires ${DIR}/45_gcd.def

# extRules file location (OpenRCX RC tech file)
set extRules patterns.rules
# Spef output filename
set spef_out gcd.spef

# Load via resistance info
source via_resistance.tcl

# Run Extraction
define_process_corner -ext_model_index 0 X
extract_parasitics -ext_model_file $extRules

# Write Spef
write_spef $spef_out

exit

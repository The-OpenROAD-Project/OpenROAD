source "helpers.tcl"

# Ensure that we can read the generated abstract lef

read_lef "Nangate45/Nangate45.lef"
read_lef "Nangate45/Nangate45_stdcell.lef"

read_lef gcd_abstract_lef.lefok

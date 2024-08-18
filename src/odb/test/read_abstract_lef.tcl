source "helpers.tcl"

# Ensure that we can read the generated abstract lef

read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"

read_lef gcd_abstract_lef.lefok

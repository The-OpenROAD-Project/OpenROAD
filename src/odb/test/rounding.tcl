# Verify that no rounding happens when writing LEF coordinates.
# In particular: 2000.172 not 2000.17

source "helpers.tcl"

# Open database, load lef and design

read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
read_def "data/rounding.def"

set lef_file [make_result_file rounding.lef]

write_abstract_lef $lef_file

diff_file $lef_file "rounding.lefok"

source "helpers.tcl"

# Open database, load lef and design

read_lef "data/Nangate45/NangateOpenCellLibrary.mod.lef"
read_def "abstract_origin.def"

set lef_file [make_result_file abstract_origin.lef]

write_abstract_lef -bloat_occupied_layers $lef_file

diff_file $lef_file "abstract_origin.lefok"

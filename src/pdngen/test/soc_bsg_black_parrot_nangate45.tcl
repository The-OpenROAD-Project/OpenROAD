source "helpers.tcl"

read_lef nangate45/NangateOpenCellLibrary.mod.lef
read_lef soc_bsg_black_parrot_nangate45/dummy_pads.lef

read_def soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.init.def

pdngen soc_bsg_black_parrot_nangate45/PDN.cfg -verbose

set def_file results/soc_bsg_black_parrot_nangate45_pdn.def
write_def $def_file

diff_files $def_file soc_bsg_black_parrot_nangate45.defok

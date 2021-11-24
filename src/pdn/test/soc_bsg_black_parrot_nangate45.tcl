source "helpers.tcl"

read_lef ../../../test/Nangate45/Nangate45.lef
read_lef dummy_pads.lef

read_def soc_bsg_black_parrot_nangate45/soc_bsg_black_parrot.init.def

pdngen soc_bsg_black_parrot_nangate45/PDN.cfg -verbose

set def_file results/soc_bsg_black_parrot_nangate45_pdn.def
write_def $def_file

diff_files $def_file soc_bsg_black_parrot_nangate45.defok

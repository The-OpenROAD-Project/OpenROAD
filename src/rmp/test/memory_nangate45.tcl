source "helpers.tcl"

read_liberty ./Nangate45/fakeram45_64x7.lib
read_liberty ./Nangate45/Nangate45_typ.lib
read_lef ./Nangate45/Nangate45.lef
read_lef ./Nangate45/fakeram45_64x7.lef
read_lef ./Nangate45/Nangate45_stdcell.lef
read_verilog ./memory_nangate45.v
link_design memory_nangate45
read_sdc ./memory_nangate45.sdc

# Unset dont use for tie cells
unset_dont_use *LOGIC*

resynth
report_checks

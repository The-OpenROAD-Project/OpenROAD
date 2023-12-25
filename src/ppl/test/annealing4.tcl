# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_simulated_annealing -max_iterations 2000 -alpha 0.985
place_pins -hor_layers metal3 -ver_layers metal4 -annealing

set def_file [make_result_file annealing4.def]

write_def $def_file

diff_file annealing4.defok $def_file

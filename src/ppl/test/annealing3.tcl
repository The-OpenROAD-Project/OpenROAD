# gcd_nangate45 IO placement
source "helpers.tcl"
read_lef Nangate45/Nangate45.lef
read_def gcd.def

set_simulated_annealing -temperature 10.0 -max_iterations 10000 -perturb_per_iter 500 -alpha 0.95
place_pins -hor_layers metal3 -ver_layers metal4 -annealing

set def_file [make_result_file annealing3.def]

write_def $def_file

diff_file annealing3.defok $def_file

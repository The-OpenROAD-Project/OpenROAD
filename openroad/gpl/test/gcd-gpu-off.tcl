# --config=gpu build with ENABLE_GPU=0 and no visible CUDA device:
# global_placement must stop with GPL-0336, not a raw Kokkos/CUDA error.
source helpers.tcl
read_lef ./nangate45.lef
read_def ./gcd.def

global_placement -init_density_penalty 0.01 -skip_initial_place

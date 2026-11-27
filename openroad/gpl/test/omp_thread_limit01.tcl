# Placement with more threads requested than OpenMP will supply.
# test/BUILD sets OMP_THREAD_LIMIT=1, so OpenMP caps every parallel region at
# one thread while gpl is asked for two.
source helpers.tcl
set test_name omp_thread_limit01
read_lef ./nangate45.lef
read_def ./gcd.def

set_thread_count 2
global_placement -init_density_penalty 0.01 -skip_initial_place

set def_file [make_result_file $test_name.def]
write_def $def_file
check "placement under OMP_THREAD_LIMIT=1 matches the single-thread DEF" \
  {diff_files $def_file gcd-single-thread.defok} 0
exit_summary

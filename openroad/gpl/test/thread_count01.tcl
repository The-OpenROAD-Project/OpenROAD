# Multi-threaded placement in a fresh process. The first placement asks for
# eight threads, so a host backend whose pool size is fixed at first use
# (Kokkos Threads) starts with several workers instead of the single worker an
# earlier one-thread placement would leave behind. The result must match the
# single-thread placement exactly.
source helpers.tcl
set test_name thread_count01
read_lef ./nangate45.lef
read_def ./gcd.def

set_thread_count 8
global_placement -init_density_penalty 0.01 -skip_initial_place

set def_file [make_result_file $test_name.def]
write_def $def_file
check "8-thread placement matches the single-thread DEF" \
  {diff_files $def_file gcd-single-thread.defok} 0
exit_summary

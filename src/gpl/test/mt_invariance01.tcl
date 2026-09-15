# Global placement must be bit-identical at any thread count. The density
# scatter accumulates into the integer bin fields with each addend truncated
# before it is added, so the sum is order-independent; this asserts that by
# running core01 threaded against core01's single-threaded golden.
source helpers.tcl
set test_name mt_invariance01
read_lef ./nangate45.lef
read_def ./core01.def

set_thread_count 8
global_placement -density 0.6 -init_density_penalty 0.01 -skip_initial_place

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file core01.defok

# PARALLEL_EVAL with worker threads: the workers fold back in target order, so
# the log is the one of the single-thread run (repair_setup_parallel_eval.ok).
source "helpers.tcl"
set_thread_count 4
source "repair_setup_parallel_eval.tcl"

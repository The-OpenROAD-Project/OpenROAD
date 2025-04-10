source "helpers.tcl"
set_thread_count [expr [cpu_count] / 4]
run_worker -host 127.0.0.1 -port 1111
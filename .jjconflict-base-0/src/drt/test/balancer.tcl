source "helpers.tcl"
add_worker_address -host 127.0.0.1 -port 1111
add_worker_address -host 127.0.0.1 -port 1112
run_load_balancer -host 127.0.0.1 -port 1234

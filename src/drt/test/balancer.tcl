add_worker_address -host 127.0.0.1 -port 1111 -threads 8
add_worker_address -host 127.0.0.1 -port 1112 -threads 8
run_load_balancer -host 127.0.0.1 -port 1234

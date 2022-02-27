set_thread_count [exec getconf _NPROCESSORS_ONLN]
run_worker -host 127.0.0.1 -port 1111 -threads 8
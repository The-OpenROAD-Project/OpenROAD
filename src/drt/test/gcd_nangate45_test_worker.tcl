set_debug_level DRT autotuner 1
detailed_route_debug -dr -net _132_
detailed_route_worker_debug -follow_guide 0 -ripup_mode 0
detailed_route_run_worker results/design.db gcd_nangate45.route_guide results/iter1_x79800_y50400.globals results/iter1_x79800_y50400.worker results/updates_pre_init.bin
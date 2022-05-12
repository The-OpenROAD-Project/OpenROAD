set_debug_level DRT autotuner 1
detailed_route_debug -dr
detailed_route_worker_debug -maze_end_iter 1 -drc_cost 8 -marker_cost 8 -follow_guide 1 -ripup_mode 1
detailed_route_run_worker results/design.db \
                          gcd_nangate45.route_guide \
                          results/globals.bin \
                          results/worker.bin \
                          results/updates.bin \
                          results/worker.drc.rpt
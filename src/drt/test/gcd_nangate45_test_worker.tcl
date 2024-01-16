source "helpers.tcl"
set_debug_level DRT autotuner 1
detailed_route_debug -dr
detailed_route_worker_debug -maze_end_iter 1 -drc_cost 8 -marker_cost 8 -follow_guide 1 -ripup_mode 1
detailed_route_run_worker -dump_dir results \
                          -worker_dir workerx67200_y37800 \
                          -drc_rpt results/worker.drc.rpt
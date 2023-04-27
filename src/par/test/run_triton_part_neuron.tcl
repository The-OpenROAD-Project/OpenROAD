triton_part_hypergraph -hypergraph_file neuron.hgr  \
-num_parts 3 -balance_constraint 5 -seed 1 \
-thr_coarsen_vertices 10 \
-thr_coarsen_hyperedge_size_skip 1000 \
-max_moves 50 \
-num_initial_solutions 100 
exit

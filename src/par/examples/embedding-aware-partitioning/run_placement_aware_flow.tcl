############################################################################################
### Scripts for placement-aware partitioning
### DATE: 2023-05-22
############################################################################################

### ----------------------------------------------------------------------------------------
### Set global variable here
set num_parts 2
set balance_constraint 2
set seed 0
set design sparcT1_chip2
set hypergraph_file "${design}.hgr"
set placement_file "${design}.hgr.ubfactor.2.numparts.2.embedding.dat"
set solution_file "${design}.hgr.part.${num_parts}"

### ----------------------------------------------------------------------------------------
### TritonPart with placement information
### Here we recommend using a small placement_wt_factors
puts "Start TritonPart for hypergraph partitioning with placement"
triton_part_hypergraph  -hypergraph_file $hypergraph_file -num_parts $num_parts \
                        -balance_constraint $balance_constraint \
                        -seed $seed  \
                        -placement_file ${placement_file} -placement_wt_factors { 0.00005 0.00005 } \
                        -placement_dimension 2

exit
### Finish 

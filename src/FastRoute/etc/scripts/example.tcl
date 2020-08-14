fr_import_lef "path/to/example.lef"
fr_import_def "path/to/example.def"
set_output_file "path/to/output.guide"

set_capacity_adjustment 0.X
set_layer_adjustment M 0.N
set_region_adjustment lx ly ux uy layer adjustment
set_min_layer Y
set_max_layer Z
set_unidirectional_routing B

run

exit

# fr_import_lef:                string input. set the lef file that will be loaded
# fr_import_def:                string input. set the def file that will be loaded
# set_output_file:              string input. indicate the name of the generated guides file. do not need ".guide" extension

# set_pitches_in_tile:          integer input. indicate the number of routing tracks per tile
# set_capacity_adjustment:      float input. indicate the percentage reduction of each edge. optional
# set_layer_adjustment:         integer, float inputs. indicate the percentage reduction of each edge in a specified layer
# set_region_adjustment:        int, int, int, int, int, float. indicate the percentage reduction of each edge in a specified region
# set_min_layer:                integer input. indicate the min routing layer available for FastRoute. optional
# set_max_layer:                integer input. indicate the max routing layer available for FastRoute. optional
# set_unidirectional_routing:   boolean input. indicate if unidirectional routing is activated. optional


# start_fastroute:              initialize FastRoute4-lefdef structures
# run_fastroute:                run only FastRoute4.1 algorithm, without write guides
# write_guides:                 write guides file. Should be called only after run_fastroute
# run:                          execute FastRoute4-lefdef flow
source helpers.tcl
set test_name cluster_place01
read_lef ./nangate45.lef
read_def ./$test_name.def

# connections to net _110_
placement_cluster [list _354_ _365_ _420_ _429_ _434_ _442_ _450_ _455_ \
                 _463_ _468_ _475_]

global_placement -density 0.6 -init_density_penalty 0.01 -skip_initial_place

set def_file [make_result_file $test_name.def]
write_def $def_file
diff_file $def_file $test_name.defok

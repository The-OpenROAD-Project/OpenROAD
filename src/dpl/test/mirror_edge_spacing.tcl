source "helpers.tcl"
# optimize_mirroring must not create cell edge spacing violations.
# u2 abuts u1, so mirroring it would put its VERTICAL_EDGE_2 edge right on
# top of u1's, violating the 0.5um CELLEDGESPACINGTABLE rule. u3 is isolated,
# so it is free to mirror.
read_lef Nangate45/Nangate45.lef
read_lef mirror_edge_spacing.lef
read_def mirror_edge_spacing.def
optimize_mirroring
check_placement -verbose

set def_file [make_result_file mirror_edge_spacing.def]
write_def $def_file
diff_file $def_file mirror_edge_spacing.defok

source "helpers.tcl"

read_db ispd18_sample_reroute.odb
detailed_route -output_drc [make_result_file ispd18_sample.output.drc.rpt] \
  -output_maze [make_result_file ispd18_sample.output.maze.log] \
  -output_guide_coverage [make_result_file ispd18_sample.coverage.csv] \
  -verbose 0

set def_file [make_result_file ispd18_sample_reroute.def]
write_def $def_file
diff_files ispd18_sample_reroute.defok $def_file


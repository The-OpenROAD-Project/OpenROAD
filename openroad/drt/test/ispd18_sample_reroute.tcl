source "helpers.tcl"

read_lef testcase/ispd18_sample/ispd18_sample.input.lef
read_def ispd18_sample_reroute.input.def
read_guides testcase/ispd18_sample/ispd18_sample.input.guide
set_routing_layers -signal Metal1-Metal9
odb::dbMarkerCategory_fromTR [[ord::get_db] getChip] DRC \
  ispd18_sample_reroute.drc.rpt
detailed_route -output_drc [make_result_file ispd18_sample.output.drc.rpt] \
  -output_maze [make_result_file ispd18_sample.output.maze.log] \
  -output_guide_coverage [make_result_file ispd18_sample.coverage.csv] \
  -verbose 0

set def_file [make_result_file ispd18_sample_reroute.def]
write_def $def_file
diff_files ispd18_sample_reroute.defok $def_file

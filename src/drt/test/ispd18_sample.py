from openroad import Design, Tech
import helpers
import drt_aux

tech = Tech()
tech.readLef("testcase/ispd18_sample/ispd18_sample.input.lef")
design = Design(tech)
design.readDef("testcase/ispd18_sample/ispd18_sample.input.def")

gr = design.getGlobalRouter()
gr.readGuides("testcase/ispd18_sample/ispd18_sample.input.guide")

drt_aux.detailed_route(design,
                       output_drc="results/ispd18_sample.output.drc.rpt",
                       output_maze="results/ispd18_sample.output.maze.log",
                       output_guide_coverage="results/ispd18_sample.coverage.csv",
                       verbose=0)

def_file = helpers.make_result_file("ispd18_sample.def")
design.writeDef(def_file)
helpers.diff_files("ispd18_sample.defok", def_file)

# tr.readParams(param_file)
# tr.main()

# source "helpers.tcl"

# read_lef testcase/ispd18_sample/ispd18_sample.input.lef
# read_def testcase/ispd18_sample/ispd18_sample.input.def
# read_guides testcase/ispd18_sample/ispd18_sample.input.guide
# detailed_route -output_drc results/ispd18_sample.output.drc.rpt \
#                -output_maze results/ispd18_sample.output.maze.log \
#                -output_guide_coverage results/ispd18_sample.coverage.csv \
#                -verbose 0

# set def_file [make_result_file ispd18_sample.def]
# write_def $def_file
# diff_files ispd18_sample.defok $def_file

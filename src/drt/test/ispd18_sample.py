from openroad import Design, Tech
import helpers
import drt_aux

bazel_working_dir = "/_main/src/drt/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("testcase/ispd18_sample/ispd18_sample.input.lef")
design = helpers.make_design(tech)
design.readDef("testcase/ispd18_sample/ispd18_sample.input.def")

gr = design.getGlobalRouter()
gr.readGuides("testcase/ispd18_sample/ispd18_sample.input.guide")

drc_file = helpers.make_result_file("ispd18_sample.output.drc.rpt")
maze_file = helpers.make_result_file("ispd18_sample.output.maze.log")
guide_coverage_file = helpers.make_result_file("ispd18_sample.coverage.csv")

drt_aux.detailed_route(
    design,
    output_drc=drc_file,
    output_maze=maze_file,
    output_guide_coverage=guide_coverage_file,
    verbose=0,
)

def_file = helpers.make_result_file("ispd18_sample.def")
design.writeDef(def_file)
helpers.diff_files("ispd18_sample.defok", def_file)

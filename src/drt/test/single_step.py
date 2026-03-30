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

drc_file = helpers.make_result_file("single_step.output.drc.rpt")
maze_file = helpers.make_result_file("single_step.output.maze.log")

drt_aux.detailed_route(
    design,
    output_drc=drc_file,
    output_maze=maze_file,
    verbose=0,
    single_step_dr=True,
)

drt_aux.step_dr(design, 7, 0, 3, 8, 0, 8, 0.95, 1, True)
drt_aux.step_dr(design, 7, -2, 3, 8, 8, 8, 0.95, 1, True)
drt_aux.step_dr(design, 7, -5, 3, 8, 8, 8, 0.95, 1, True)
drt_aux.step_end(design)

def_file = helpers.make_result_file("single_step.def")
design.writeDef(def_file)
helpers.diff_files("ispd18_sample.defok", def_file)

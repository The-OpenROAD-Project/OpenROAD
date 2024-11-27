from openroad import Design, Tech
import helpers
import drt_aux

tech = Tech()
tech.readLef("testcase/ispd18_sample/ispd18_sample.input.lef")
design = helpers.make_design(tech)
design.readDef("testcase/ispd18_sample/ispd18_sample.input.def")

gr = design.getGlobalRouter()
gr.readGuides("testcase/ispd18_sample/ispd18_sample.input.guide")

drt_aux.detailed_route(
    design,
    output_drc="results/ispd18_sample.output.drc.rpt",
    output_maze="results/ispd18_sample.output.maze.log",
    output_guide_coverage="results/ispd18_sample.coverage.csv",
    verbose=0,
)

def_file = helpers.make_result_file("ispd18_sample.def")
design.writeDef(def_file)
helpers.diff_files("ispd18_sample.defok", def_file)

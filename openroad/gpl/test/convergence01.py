# Tests that skipping early iterations min overflow updating produces
# convergence with -timing_driven (see NesterovPlace::doNesterovPlace)

from openroad import Design, Tech
import helpers

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()

tech.readLiberty("asap7/asap7sc7p5t_AO_RVT_FF_nldm_211120.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_INVBUF_RVT_FF_nldm_220122.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_OA_RVT_FF_nldm_211120.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_SIMPLE_RVT_FF_nldm_211120.lib.gz")
tech.readLiberty("asap7/asap7sc7p5t_SEQ_RVT_FF_nldm_220123.lib")

tech.readLef("./asap7/asap7_tech_1x_201209.lef")
tech.readLef("./asap7/asap7sc7p5t_28_R_1x_220121a.lef")

design = helpers.make_design(tech)
design.readDef("convergence01.def")

design.evalTclString("read_sdc convergence01.sdc")

design.evalTclString("source asap7/setRC.tcl")

options = helpers.PlaceOptions()
options.padLeft = 2
options.padRight = 2
options.density = 0.5
options.timingDrivenMode = True
design.getReplace().doPlace(1, options)

def_file = helpers.make_result_file("convergence01.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "convergence01.defok")

from openroad import Design, Tech
import helpers
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01-ref.def")

options = gpl.PlaceOptions()
options.initDensityPenaltyFactor = 0.01
options.referenceHpwl = 384000.0
options.initialPlaceMaxIter = 0
design.getReplace().doPlace(1, options)

def_file = helpers.make_result_file("simple01-ref.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-ref.defok")

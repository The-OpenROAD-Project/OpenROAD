from openroad import Design, Tech
import helpers
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple05.def")

options = gpl.PlaceOptions()
options.initialPlaceMaxIter = 0
design.getReplace().doPlace(1, options)

def_file = helpers.make_result_file("simple05.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple05.defok")

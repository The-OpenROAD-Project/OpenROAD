from openroad import Design, Tech
import helpers
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple06.def")

options = gpl.PlaceOptions()
design.getReplace().doInitialPlace(1)

def_file = helpers.make_result_file("simple06.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple06.defok")

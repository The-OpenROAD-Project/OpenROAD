from openroad import Design, Tech
import helpers
import gpl

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./error01.def")

options = gpl.PlaceOptions()
options.initDensityPenaltyFactor = 0.01
options.density = 0.001
options.initialPlaceMaxIter = 0
design.getReplace().doPlace(1, options)

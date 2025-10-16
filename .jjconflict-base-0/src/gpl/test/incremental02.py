from openroad import Design, Tech
import helpers
import gpl_aux

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./incremental02.def")

gpl_aux.global_placement(design, incremental=True, density=0.3, pad_left=2, pad_right=2)

def_file = helpers.make_result_file("incremental02.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "incremental02.defok")

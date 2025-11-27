from openroad import Design, Tech
import helpers

bazel_working_dir = "/_main/src/gpl/test/"
helpers.if_bazel_change_working_dir_to(bazel_working_dir)

tech = Tech()
tech.readLiberty("./library/nangate45/NangateOpenCellLibrary_typical.lib")
tech.readLef("./nangate45.lef")
design = helpers.make_design(tech)
design.readDef("./simple01-td.def")

design.evalTclString("create_clock -name core_clock -period 2 clk")

design.evalTclString("set_wire_rc -signal -layer metal3")
design.evalTclString("set_wire_rc -clock  -layer metal5")

options = helpers.PlaceOptions()
options.timingDrivenMode = True
design.getReplace().doPlace(1, options)

design.evalTclString("estimate_parasitics -placement")
design.evalTclString("report_worst_slack")

def_file = helpers.make_result_file("simple01-td.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-td.defok")

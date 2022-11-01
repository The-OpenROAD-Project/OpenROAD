from openroad import Design, Tech
import helpers
import gpl_aux

tech = Tech()
tech.readLiberty("./library/nangate45/NangateOpenCellLibrary_typical.lib")
tech.readLef("./nangate45.lef")
design = Design(tech)
design.readDef("simple01-td-tune.def")

design.evalTclString("create_clock -name core_clock -period 2 clk")
design.evalTclString("set_wire_rc -signal -layer metal3")
design.evalTclString("set_wire_rc -clock  -layer metal5")

gpl_aux.global_placement(design, timing_driven=True, timing_driven_net_reweight_overflow=[80, 70, 60, 50, 40, 30, 20])

design.evalTclString("estimate_parasitics -placement")
design.evalTclString("report_worst_slack")

def_file = helpers.make_result_file("simple01-td-tune.def")
design.writeDef(def_file)
helpers.diff_files(def_file, "simple01-td-tune.defok")

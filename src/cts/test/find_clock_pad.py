from openroad import Design, Tech
import helpers
import cts_aux

tech = Tech()
tech.readLef("Nangate45/Nangate45.lef")
tech.readLef("pad.lef")
tech.readLiberty("Nangate45/Nangate45_typ.lib")
tech.readLiberty("pad.lib")

design = helpers.make_design(tech)
design.readDef("find_clock_pad.def")

design.evalTclString("create_clock -name clk -period 10 clk1")
design.evalTclString("set_wire_rc -clock -layer metal5")

cts_aux.clock_tree_synthesis(design, buf_list="BUF_X1", obstruction_aware=True)
